/*
 * CrisperCue firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/gas.h"
#include "drivers/mass.h"
#include "drivers/optical.h"
#include "drivers/thermal.h"
#include "drivers/power.h"
#include "drivers/inference.h"
#include "drivers/ble.h"
#include "drivers/logger.h"

static const char *profile_name(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return "leafy-greens";
    case BIN_PROFILE_BERRIES: return "berries";
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return "climacteric-fruit";
    default: return "unknown";
    }
}

static const char *alert_name(alert_level_t level)
{
    switch (level) {
    case ALERT_NONE: return "none";
    case ALERT_INFO: return "info";
    case ALERT_CAUTION: return "caution";
    case ALERT_WARNING: return "warning";
    case ALERT_CRITICAL: return "critical";
    default: return "unknown";
    }
}

static void snapshot_init(crisper_snapshot_t *snapshot, bin_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    thermal_init(&snapshot->thermal, profile);
    gas_init(&snapshot->gas, profile);
    mass_init(&snapshot->mass, profile);
    optical_init(&snapshot->optical, profile);
    power_init(&snapshot->power);
    inference_init(&snapshot->inference);
}

static void maybe_log_alert(const crisper_snapshot_t *current, const crisper_snapshot_t *previous, event_log_t *events)
{
    char detail[128];
    if (current->inference.alert == ALERT_NONE) {
        return;
    }
    if (current->inference.alert == previous->inference.alert &&
        current->inference.spoilage_risk - previous->inference.spoilage_risk < 0.08f &&
        current->gas.ethylene_ppm - previous->gas.ethylene_ppm < 0.06f) {
        return;
    }

    snprintf(detail, sizeof(detail),
             "reason=%s stage=%s ethylene=%.3f co2=%.0f mass=%.1f mold=%.2f dew=%.2f",
             inference_primary_reason(&current->inference),
             current->inference.stage,
             current->gas.ethylene_ppm,
             current->gas.co2_ppm,
             current->mass.tray_mass_g,
             current->optical.mold_signature,
             current->thermal.dew_margin_c);

    if (current->optical.mold_signature > 0.75f) {
        logger_push_event(events, current->cycle_index, current->inference.alert, "MOLD_RISK", detail);
    } else if (current->gas.ethylene_ppm > 0.70f) {
        logger_push_event(events, current->cycle_index, current->inference.alert, "RIPENING", detail);
    } else if (current->mass.moisture_loss_percent > 12.0f) {
        logger_push_event(events, current->cycle_index, current->inference.alert, "DEHYDRATE", detail);
    } else {
        logger_push_event(events, current->cycle_index, current->inference.alert, "INVENTORY", detail);
    }
}

static void print_snapshot(const crisper_snapshot_t *snapshot)
{
    printf("cycle=%02u temp=%4.1fC produce=%4.1fC door=%s gas=%s co2=%5.0f eth=%4.3f hum=%4.1f mass=%6.1fg use=%s color=%4.2f mold=%4.2f fresh=%5.1f stage=%s alert=%s\n",
           snapshot->cycle_index,
           snapshot->thermal.air_temp_c,
           snapshot->thermal.produce_temp_c,
           thermal_door_label(&snapshot->thermal),
           gas_air_quality_label(&snapshot->gas),
           snapshot->gas.co2_ppm,
           snapshot->gas.ethylene_ppm,
           snapshot->gas.humidity_rh,
           snapshot->mass.tray_mass_g,
           mass_usage_label(&snapshot->mass),
           snapshot->optical.color_index,
           snapshot->optical.mold_signature,
           snapshot->inference.freshness_score,
           snapshot->inference.stage,
           alert_name(snapshot->inference.alert));
}

static void print_register_map(const crisper_snapshot_t *snapshot)
{
    printf("\nRegister snapshot for CrisperCue by %s\n", CRISPERCUE_AUTHOR);
    printf("  REG_PWR_STATUS           [0x%04X] = %u\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV       [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PERCENT  [0x%04X] = %.0f\n", REG_PWR_BATTERY_PERCENT, snapshot->power.battery_percent);
    printf("  REG_PWR_CURRENT_MA       [0x%04X] = %.0f\n", REG_PWR_CURRENT_MA, snapshot->power.current_ma);
    printf("  REG_GAS_CO2_X10          [0x%04X] = %.0f\n", REG_GAS_CO2_X10, snapshot->gas.co2_ppm * 10.0f);
    printf("  REG_GAS_ETHYLENE_X100    [0x%04X] = %.0f\n", REG_GAS_ETHYLENE_X100, snapshot->gas.ethylene_ppm * 100.0f);
    printf("  REG_GAS_VOC_X10          [0x%04X] = %.0f\n", REG_GAS_VOC_X10, snapshot->gas.voc_index * 10.0f);
    printf("  REG_GAS_OXYGEN_X100      [0x%04X] = %.0f\n", REG_GAS_OXYGEN_X100, snapshot->gas.oxygen_percent * 100.0f);
    printf("  REG_GAS_HUMIDITY_X100    [0x%04X] = %.0f\n", REG_GAS_HUMIDITY_X100, snapshot->gas.humidity_rh * 100.0f);
    printf("  REG_GAS_PURGE_X100       [0x%04X] = %.0f\n", REG_GAS_PURGE_X100, snapshot->gas.purge_efficiency * 100.0f);
    printf("  REG_MASS_TRAY_G          [0x%04X] = %.0f\n", REG_MASS_TRAY_G, snapshot->mass.tray_mass_g);
    printf("  REG_MASS_LOSS_G_X10      [0x%04X] = %.0f\n", REG_MASS_LOSS_G_X10, snapshot->mass.daily_loss_g * 10.0f);
    printf("  REG_MASS_MOISTURE_X100   [0x%04X] = %.0f\n", REG_MASS_MOISTURE_X100, snapshot->mass.moisture_loss_percent * 100.0f);
    printf("  REG_MASS_USAGE_X100      [0x%04X] = %.0f\n", REG_MASS_USAGE_X100, snapshot->mass.usage_velocity * 100.0f);
    printf("  REG_OPT_COLOR_X100       [0x%04X] = %.0f\n", REG_OPT_COLOR_X100, snapshot->optical.color_index * 100.0f);
    printf("  REG_OPT_CHLORO_X100      [0x%04X] = %.0f\n", REG_OPT_CHLORO_X100, snapshot->optical.chlorophyll_index * 100.0f);
    printf("  REG_OPT_BRUISE_X100      [0x%04X] = %.0f\n", REG_OPT_BRUISE_X100, snapshot->optical.bruise_probability * 100.0f);
    printf("  REG_OPT_MOLD_X100        [0x%04X] = %.0f\n", REG_OPT_MOLD_X100, snapshot->optical.mold_signature * 100.0f);
    printf("  REG_OPT_GLOSS_X100       [0x%04X] = %.0f\n", REG_OPT_GLOSS_X100, snapshot->optical.surface_gloss * 100.0f);
    printf("  REG_TH_AIR_C_X100        [0x%04X] = %.0f\n", REG_TH_AIR_C_X100, snapshot->thermal.air_temp_c * 100.0f);
    printf("  REG_TH_PRODUCE_C_X100    [0x%04X] = %.0f\n", REG_TH_PRODUCE_C_X100, snapshot->thermal.produce_temp_c * 100.0f);
    printf("  REG_TH_DEW_MARGIN_X100   [0x%04X] = %.0f\n", REG_TH_DEW_MARGIN_X100, snapshot->thermal.dew_margin_c * 100.0f);
    printf("  REG_TH_COMPRESSOR_X100   [0x%04X] = %.0f\n", REG_TH_COMPRESSOR_X100, snapshot->thermal.compressor_cycles * 100.0f);
    printf("  REG_TH_OPEN_MIN_X10      [0x%04X] = %.0f\n", REG_TH_OPEN_MIN_X10, snapshot->thermal.drawer_open_minutes * 10.0f);
    printf("  REG_INFER_FRESHNESS_X100 [0x%04X] = %.0f\n", REG_INFER_FRESHNESS_X100, snapshot->inference.freshness_score * 100.0f);
    printf("  REG_INFER_SPOILAGE_X100  [0x%04X] = %.0f\n", REG_INFER_SPOILAGE_X100, snapshot->inference.spoilage_risk * 100.0f);
    printf("  REG_INFER_RECIPE_X100    [0x%04X] = %.0f\n", REG_INFER_RECIPE_X100, snapshot->inference.recipe_urgency * 100.0f);
    printf("  REG_INFER_VENT_X100      [0x%04X] = %.0f\n", REG_INFER_VENT_X100, snapshot->inference.ventilation_demand * 100.0f);
    printf("  REG_INFER_VALUE_CENTS    [0x%04X] = %.0f\n", REG_INFER_VALUE_CENTS, snapshot->inference.shopper_value_left_usd * 100.0f);
    printf("  REG_ALERT_LEVEL          [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->inference.alert);
}

static void print_final_assessment(const crisper_snapshot_t *snapshot)
{
    printf("\nFinal assessment\n");
    printf("  Device: %s\n", CRISPERCUE_DEVICE_NAME);
    printf("  Author: %s\n", CRISPERCUE_AUTHOR);
    printf("  Freshness score: %.1f\n", snapshot->inference.freshness_score);
    printf("  Spoilage risk: %.2f\n", snapshot->inference.spoilage_risk);
    printf("  Recipe urgency: %.2f\n", snapshot->inference.recipe_urgency);
    printf("  Ventilation demand: %.2f\n", snapshot->inference.ventilation_demand);
    printf("  Shopper value left: $%.2f\n", snapshot->inference.shopper_value_left_usd);
    printf("  Stage: %s\n", snapshot->inference.stage);
    printf("  Primary reason: %s\n", inference_primary_reason(&snapshot->inference));
    printf("  Suggested action: %s\n",
           snapshot->inference.alert == ALERT_CRITICAL ?
           "Remove visibly affected produce, wash the bin liner, and trigger the sanitation routine." :
           (snapshot->inference.alert == ALERT_WARNING ?
            "Move produce to the front of the meal plan and open the active purge louver." :
            "Continue tracking; produce is still within usable quality limits."));
}

int main(void)
{
    crisper_snapshot_t previous;
    crisper_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[384];
    char report_packet[512];
    size_t used;
    uint32_t cycle;
    const bin_profile_t profile = BIN_PROFILE_CLIMACTERIC_FRUIT;

    printf("%s firmware by %s\n", CRISPERCUE_DEVICE_NAME, CRISPERCUE_AUTHOR);
    printf("Firmware version: %s | profile=%s\n\n", CRISPERCUE_FIRMWARE_VERSION, profile_name(profile));

    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);

    for (cycle = 1u; cycle <= CRISPERCUE_LOOP_COUNT; ++cycle) {
        current = previous;
        current.cycle_index = cycle;
        thermal_sample(&current.thermal, profile, cycle);
        gas_sample(&current.gas, profile, cycle, &current.thermal);
        mass_sample(&current.mass, profile, cycle, &current.gas);
        optical_sample(&current.optical, profile, cycle, &current.gas, &current.mass);
        inference_update(&current.inference, profile, &current, &previous);
        power_update(&current.power, &previous.power, &current);
        maybe_log_alert(&current, &previous, &events);
        logger_push_snapshot(&snapshots, &current);
        print_snapshot(&current);
        previous = current;
    }

    used = ble_build_status_packet(&previous, status_packet, sizeof(status_packet));
    printf("\nBLE status packet (%zu bytes):\n%s\n", used, status_packet);
    used = ble_build_report_packet(&previous, &events, report_packet, sizeof(report_packet));
    printf("\nBLE report packet (%zu bytes):\n%s\n", used, report_packet);
    print_register_map(&previous);
    logger_print_summary(&events, &snapshots);
    print_final_assessment(&previous);
    return 0;
}
