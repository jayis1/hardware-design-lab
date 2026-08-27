/*
 * SealBeat firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/acoustic.h"
#include "drivers/door.h"
#include "drivers/seal.h"
#include "drivers/thermal.h"
#include "drivers/power.h"
#include "drivers/inference.h"
#include "drivers/ble.h"
#include "drivers/logger.h"

float sb_clampf(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

float sb_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float sb_profile_bias(appliance_profile_t profile, float residential, float freezer, float pharmacy)
{
    switch (profile) {
    case APPLIANCE_PROFILE_RESIDENTIAL_FRIDGE: return residential;
    case APPLIANCE_PROFILE_UPRIGHT_FREEZER: return freezer;
    case APPLIANCE_PROFILE_PHARMACY_COOLER: return pharmacy;
    default: return residential;
    }
}

const char *sb_profile_name(appliance_profile_t profile)
{
    switch (profile) {
    case APPLIANCE_PROFILE_RESIDENTIAL_FRIDGE: return "residential-fridge";
    case APPLIANCE_PROFILE_UPRIGHT_FREEZER: return "upright-freezer";
    case APPLIANCE_PROFILE_PHARMACY_COOLER: return "pharmacy-cooler";
    default: return "unknown";
    }
}

const char *sb_alert_name(alert_level_t level)
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

static void snapshot_init(appliance_snapshot_t *snapshot, appliance_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    acoustic_init(&snapshot->acoustic, profile);
    door_init(&snapshot->door, profile);
    seal_init(&snapshot->seal, profile);
    thermal_init(&snapshot->thermal, profile);
    power_init(&snapshot->power, profile);
    inference_init(&snapshot->inference);
    snapshot->minute_index = 0u;
}

static void maybe_log_alert(const appliance_snapshot_t *current, const appliance_snapshot_t *previous, event_log_t *events)
{
    char detail[120];
    if (current->inference.alert == ALERT_NONE) return;
    if (current->inference.alert == previous->inference.alert && current->inference.maintenance_priority - previous->inference.maintenance_priority < 0.05f) return;
    snprintf(detail, sizeof(detail),
             "reason=%s seal=%.2f safety=%.2f hinge=%.2f edge=%s door=%s thermal=%s",
             inference_primary_reason(current),
             current->inference.seal_integrity,
             current->inference.safety_confidence,
             current->inference.hinge_wear,
             seal_edge_label(&current->seal),
             door_pattern_label(&current->door),
             thermal_status_label(&current->thermal));
    if (current->inference.safety_confidence < 0.56f) {
        logger_push_event(events, current->minute_index, current->inference.alert, "SAFETY", detail);
    } else if (current->inference.hinge_wear > 0.48f) {
        logger_push_event(events, current->minute_index, current->inference.alert, "HINGE", detail);
    } else {
        logger_push_event(events, current->minute_index, current->inference.alert, "SEAL", detail);
    }
}

static void print_snapshot(const appliance_snapshot_t *snapshot)
{
    printf("minute=%02u seal=%4.2f safety=%4.2f hinge=%4.2f close=%4.2f gap=%4.2fmm rebound=%4.2fC tau=%5.1fs batt=%5.1f%% edge=%s alert=%s\n",
           snapshot->minute_index,
           snapshot->inference.seal_integrity,
           snapshot->inference.safety_confidence,
           snapshot->inference.hinge_wear,
           snapshot->inference.closure_quality,
           snapshot->seal.final_gap_mm,
           snapshot->thermal.warm_rebound_c,
           snapshot->thermal.recovery_tau_s,
           snapshot->power.battery_percent,
           seal_edge_label(&snapshot->seal),
           sb_alert_name(snapshot->inference.alert));
}

static void print_register_map(const appliance_snapshot_t *snapshot)
{
    printf("\nRegister snapshot for SealBeat by %s\n", SEALBEAT_AUTHOR);
    printf("  REG_PWR_STATUS           [0x%04X] = %.0f\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV       [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PERCENT  [0x%04X] = %.0f\n", REG_PWR_BATTERY_PERCENT, snapshot->power.battery_percent);
    printf("  REG_PWR_EST_DAYS         [0x%04X] = %.0f\n", REG_PWR_EST_DAYS, snapshot->power.estimated_days_left);
    printf("  REG_ACOUSTIC_LATCH_X100  [0x%04X] = %.0f\n", REG_ACOUSTIC_LATCH_X100, snapshot->acoustic.latch_sharpness * 100.0f);
    printf("  REG_ACOUSTIC_HARM_X100   [0x%04X] = %.0f\n", REG_ACOUSTIC_HARM_X100, snapshot->acoustic.compressor_harmonic * 100.0f);
    printf("  REG_ACOUSTIC_VIBE_X100   [0x%04X] = %.0f\n", REG_ACOUSTIC_VIBE_X100, snapshot->acoustic.frame_vibration * 100.0f);
    printf("  REG_ACOUSTIC_BURDEN_X100 [0x%04X] = %.0f\n", REG_ACOUSTIC_BURDEN_X100, snapshot->acoustic.compressor_burden * 100.0f);
    printf("  REG_DOOR_ANGLE_X100      [0x%04X] = %.0f\n", REG_DOOR_ANGLE_X100, snapshot->door.door_angle_deg * 100.0f);
    printf("  REG_DOOR_DWELL_S         [0x%04X] = %.0f\n", REG_DOOR_DWELL_S, snapshot->door.dwell_open_seconds);
    printf("  REG_DOOR_BOUNCE_X100     [0x%04X] = %.0f\n", REG_DOOR_BOUNCE_X100, snapshot->door.bounce_count * 100.0f);
    printf("  REG_DOOR_HINGE_X100      [0x%04X] = %.0f\n", REG_DOOR_HINGE_X100, snapshot->door.hinge_skew * 100.0f);
    printf("  REG_DOOR_CYCLES          [0x%04X] = %u\n", REG_DOOR_CYCLES, snapshot->door.cycle_count);
    printf("  REG_SEAL_TOP_X100        [0x%04X] = %.0f\n", REG_SEAL_TOP_X100, snapshot->seal.top_edge_score * 100.0f);
    printf("  REG_SEAL_LATCH_X100      [0x%04X] = %.0f\n", REG_SEAL_LATCH_X100, snapshot->seal.latch_edge_score * 100.0f);
    printf("  REG_SEAL_BOTTOM_X100     [0x%04X] = %.0f\n", REG_SEAL_BOTTOM_X100, snapshot->seal.bottom_edge_score * 100.0f);
    printf("  REG_SEAL_HINGE_X100      [0x%04X] = %.0f\n", REG_SEAL_HINGE_X100, snapshot->seal.hinge_edge_score * 100.0f);
    printf("  REG_SEAL_COMP_X100       [0x%04X] = %.0f\n", REG_SEAL_COMP_X100, snapshot->seal.compression_uniformity * 100.0f);
    printf("  REG_SEAL_PULL_X100       [0x%04X] = %.0f\n", REG_SEAL_PULL_X100, snapshot->seal.magnetic_pull * 100.0f);
    printf("  REG_SEAL_GAP_X100        [0x%04X] = %.0f\n", REG_SEAL_GAP_X100, snapshot->seal.final_gap_mm * 100.0f);
    printf("  REG_SEAL_VECTOR_X100     [0x%04X] = %.0f\n", REG_SEAL_VECTOR_X100, snapshot->seal.leak_vector * 100.0f);
    printf("  REG_THERM_EDGE_X100      [0x%04X] = %.0f\n", REG_THERM_EDGE_X100, snapshot->thermal.edge_temp_c * 100.0f);
    printf("  REG_THERM_COMP_X100      [0x%04X] = %.0f\n", REG_THERM_COMP_X100, snapshot->thermal.compartment_temp_c * 100.0f);
    printf("  REG_THERM_REBOUND_X100   [0x%04X] = %.0f\n", REG_THERM_REBOUND_X100, snapshot->thermal.warm_rebound_c * 100.0f);
    printf("  REG_THERM_TAU_X10        [0x%04X] = %.0f\n", REG_THERM_TAU_X10, snapshot->thermal.recovery_tau_s * 10.0f);
    printf("  REG_THERM_FROST_X100     [0x%04X] = %.0f\n", REG_THERM_FROST_X100, snapshot->thermal.frost_risk * 100.0f);
    printf("  REG_THERM_SAFETY_X100    [0x%04X] = %.0f\n", REG_THERM_SAFETY_X100, snapshot->thermal.safety_margin * 100.0f);
    printf("  REG_INFER_SEAL_X100      [0x%04X] = %.0f\n", REG_INFER_SEAL_X100, snapshot->inference.seal_integrity * 100.0f);
    printf("  REG_INFER_SAFETY_X100    [0x%04X] = %.0f\n", REG_INFER_SAFETY_X100, snapshot->inference.safety_confidence * 100.0f);
    printf("  REG_INFER_HINGE_X100     [0x%04X] = %.0f\n", REG_INFER_HINGE_X100, snapshot->inference.hinge_wear * 100.0f);
    printf("  REG_INFER_PRIORITY_X100  [0x%04X] = %.0f\n", REG_INFER_PRIORITY_X100, snapshot->inference.maintenance_priority * 100.0f);
    printf("  REG_INFER_ENERGY_X100    [0x%04X] = %.0f\n", REG_INFER_ENERGY_X100, snapshot->inference.energy_penalty * 100.0f);
    printf("  REG_INFER_SERVICE_X100   [0x%04X] = %.0f\n", REG_INFER_SERVICE_X100, snapshot->inference.service_score * 100.0f);
    printf("  REG_ALERT_LEVEL          [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->inference.alert);
}

static void print_final_assessment(const appliance_snapshot_t *snapshot, appliance_profile_t profile)
{
    const char *suggested;
    if (snapshot->inference.safety_confidence < 0.56f) {
        suggested = "Inspect temperature stability immediately, reduce door dwell, and verify compartment contents remain within safe hold range.";
    } else if (snapshot->inference.hinge_wear > 0.46f) {
        suggested = "Service hinge alignment, then re-check gasket compression before replacing the seal.";
    } else if (snapshot->seal.top_edge_score < 0.58f || snapshot->seal.bottom_edge_score < 0.58f) {
        suggested = "Clean and heat-form the weak gasket edge; replace the gasket if closure quality does not recover.";
    } else {
        suggested = "Continue monitoring; closure remains acceptable for the selected appliance profile.";
    }
    printf("\nFinal assessment\n");
    printf("  Device: %s\n", SEALBEAT_DEVICE_NAME);
    printf("  Author: %s\n", SEALBEAT_AUTHOR);
    printf("  Profile: %s\n", sb_profile_name(profile));
    printf("  Primary reason: %s\n", inference_primary_reason(snapshot));
    printf("  Seal integrity: %.2f\n", snapshot->inference.seal_integrity);
    printf("  Safety confidence: %.2f\n", snapshot->inference.safety_confidence);
    printf("  Hinge wear: %.2f\n", snapshot->inference.hinge_wear);
    printf("  Maintenance priority: %.2f\n", snapshot->inference.maintenance_priority);
    printf("  Energy penalty: %.2f\n", snapshot->inference.energy_penalty);
    printf("  Suggested action: %s\n", suggested);
}

int main(void)
{
    appliance_snapshot_t previous;
    appliance_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[SEALBEAT_MAX_PACKET];
    char report_packet[SEALBEAT_MAX_PACKET];
    size_t used;
    uint32_t minute;
    const appliance_profile_t profile = APPLIANCE_PROFILE_PHARMACY_COOLER;

    printf("%s firmware by %s\n", SEALBEAT_DEVICE_NAME, SEALBEAT_AUTHOR);
    printf("Firmware version: %s | profile=%s\n\n", SEALBEAT_FIRMWARE_VERSION, sb_profile_name(profile));

    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);

    for (minute = 1u; minute <= SEALBEAT_LOOP_COUNT; ++minute) {
        current = previous;
        current.minute_index = minute;
        door_sample(&current.door, profile, minute);
        seal_sample(&current.seal, profile, minute, &current.door);
        acoustic_sample(&current.acoustic, profile, minute, &current.door, &current.seal);
        thermal_sample(&current.thermal, profile, minute, &current.door, &current.seal);
        power_update(&current.power, &previous.power, &current);
        inference_update(&current.inference, &current, &previous, profile);
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
    print_final_assessment(&previous, profile);
    return 0;
}
