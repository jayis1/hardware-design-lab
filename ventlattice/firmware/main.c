/*
 * VentLattice firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/airflow.h"
#include "drivers/pressure.h"
#include "drivers/environment.h"
#include "drivers/occupancy.h"
#include "drivers/power.h"
#include "drivers/inference.h"
#include "drivers/ble.h"
#include "drivers/logger.h"

static const char *profile_name(room_profile_t profile)
{
    switch (profile) {
    case ROOM_PROFILE_HOME_OFFICE: return "home-office";
    case ROOM_PROFILE_NURSERY: return "nursery";
    case ROOM_PROFILE_CLASSROOM: return "classroom";
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

static void snapshot_init(vent_snapshot_t *snapshot, room_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    airflow_init(&snapshot->airflow, profile);
    pressure_init(&snapshot->pressure, profile);
    environment_init(&snapshot->environment, profile);
    occupancy_init(&snapshot->occupancy, profile);
    power_init(&snapshot->power, profile);
    inference_init(&snapshot->inference);
    snapshot->hour_index = 0u;
}

static void maybe_log_alert(const vent_snapshot_t *current, const vent_snapshot_t *previous, event_log_t *events)
{
    char detail[128];
    if (current->inference.alert == ALERT_NONE) {
        return;
    }
    if (current->inference.alert == previous->inference.alert &&
        current->inference.maintenance_priority - previous->inference.maintenance_priority < 0.08f &&
        current->airflow.blockage_index - previous->airflow.blockage_index < 0.06f) {
        return;
    }

    snprintf(detail, sizeof(detail),
             "reason=%s cfm=%.1f block=%.2f stale=%.2f waste=%.2f temp=%.1f/%.1f",
             inference_primary_reason(current),
             current->airflow.airflow_cfm,
             current->airflow.blockage_index,
             current->inference.stale_air_risk,
             current->inference.comfort_waste,
             current->environment.supply_temp_c,
             current->environment.room_temp_c);

    if (current->airflow.blockage_index > 0.58f) {
        logger_push_event(events, current->hour_index, current->inference.alert, "BLOCKAGE", detail);
    } else if (current->inference.stale_air_risk > 0.56f) {
        logger_push_event(events, current->hour_index, current->inference.alert, "STALE_AIR", detail);
    } else if (current->inference.comfort_waste > 0.48f) {
        logger_push_event(events, current->hour_index, current->inference.alert, "WASTE", detail);
    } else {
        logger_push_event(events, current->hour_index, current->inference.alert, "MAINT", detail);
    }
}

static void print_snapshot(const vent_snapshot_t *snapshot)
{
    printf("hour=%02u cfm=%5.1f vel=%4.2f block=%4.2f stable=%4.2f ripple=%4.2f room=%4.1fC supply=%4.1fC voc=%5.1f occ=%4.2f service=%5.1f waste=%4.2f stale=%4.2f alert=%s mode=%s\n",
           snapshot->hour_index,
           snapshot->airflow.airflow_cfm,
           snapshot->airflow.velocity_mps,
           snapshot->airflow.blockage_index,
           snapshot->airflow.delivery_stability,
           snapshot->pressure.ripple_pa,
           snapshot->environment.room_temp_c,
           snapshot->environment.supply_temp_c,
           snapshot->environment.voc_index,
           snapshot->occupancy.presence_confidence,
           snapshot->inference.service_score,
           snapshot->inference.comfort_waste,
           snapshot->inference.stale_air_risk,
           alert_name(snapshot->inference.alert),
           airflow_state_label(&snapshot->airflow));
}

static void print_register_map(const vent_snapshot_t *snapshot)
{
    printf("\nRegister snapshot for VentLattice by %s\n", VENTLATTICE_AUTHOR);
    printf("  REG_PWR_STATUS             [0x%04X] = %.0f\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV         [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PERCENT    [0x%04X] = %.0f\n", REG_PWR_BATTERY_PERCENT, snapshot->power.battery_percent);
    printf("  REG_AIRFLOW_CFM_X10        [0x%04X] = %.0f\n", REG_AIRFLOW_CFM_X10, snapshot->airflow.airflow_cfm * 10.0f);
    printf("  REG_AIRFLOW_VELOCITY_X100  [0x%04X] = %.0f\n", REG_AIRFLOW_VELOCITY_X100, snapshot->airflow.velocity_mps * 100.0f);
    printf("  REG_AIRFLOW_BLOCK_X100     [0x%04X] = %.0f\n", REG_AIRFLOW_BLOCK_X100, snapshot->airflow.blockage_index * 100.0f);
    printf("  REG_AIRFLOW_STABLE_X100    [0x%04X] = %.0f\n", REG_AIRFLOW_STABLE_X100, snapshot->airflow.delivery_stability * 100.0f);
    printf("  REG_AIRFLOW_DELTA_PA       [0x%04X] = %.0f\n", REG_AIRFLOW_DELTA_PA, snapshot->airflow.nozzle_delta_pa);
    printf("  REG_PRESSURE_RIPPLE_X100   [0x%04X] = %.0f\n", REG_PRESSURE_RIPPLE_X100, snapshot->pressure.ripple_pa * 100.0f);
    printf("  REG_PRESSURE_FILTER_X100   [0x%04X] = %.0f\n", REG_PRESSURE_FILTER_X100, snapshot->pressure.filter_load_index * 100.0f);
    printf("  REG_PRESSURE_RESTRICT_X100 [0x%04X] = %.0f\n", REG_PRESSURE_RESTRICT_X100, snapshot->pressure.branch_restriction * 100.0f);
    printf("  REG_PRESSURE_TURB_X100     [0x%04X] = %.0f\n", REG_PRESSURE_TURB_X100, snapshot->pressure.turbulence_index * 100.0f);
    printf("  REG_ENV_SUPPLY_X100        [0x%04X] = %.0f\n", REG_ENV_SUPPLY_X100, snapshot->environment.supply_temp_c * 100.0f);
    printf("  REG_ENV_ROOM_X100          [0x%04X] = %.0f\n", REG_ENV_ROOM_X100, snapshot->environment.room_temp_c * 100.0f);
    printf("  REG_ENV_HUMIDITY_X100      [0x%04X] = %.0f\n", REG_ENV_HUMIDITY_X100, snapshot->environment.humidity_rh * 100.0f);
    printf("  REG_ENV_VOC_X10            [0x%04X] = %.0f\n", REG_ENV_VOC_X10, snapshot->environment.voc_index * 10.0f);
    printf("  REG_ENV_DEW_MARGIN_X100    [0x%04X] = %.0f\n", REG_ENV_DEW_MARGIN_X100, snapshot->environment.dew_margin_c * 100.0f);
    printf("  REG_ENV_LIGHT_LUX          [0x%04X] = %.0f\n", REG_ENV_LIGHT_LUX, snapshot->environment.light_lux);
    printf("  REG_OCCUPANCY_CONF_X100    [0x%04X] = %.0f\n", REG_OCCUPANCY_CONF_X100, snapshot->occupancy.presence_confidence * 100.0f);
    printf("  REG_OCCUPANCY_DWELL_X10    [0x%04X] = %.0f\n", REG_OCCUPANCY_DWELL_X10, snapshot->occupancy.dwell_hours * 10.0f);
    printf("  REG_OCCUPANCY_ALIGN_X100   [0x%04X] = %.0f\n", REG_OCCUPANCY_ALIGN_X100, snapshot->occupancy.occupied_alignment * 100.0f);
    printf("  REG_INFER_SERVICE_X100     [0x%04X] = %.0f\n", REG_INFER_SERVICE_X100, snapshot->inference.service_score * 100.0f);
    printf("  REG_INFER_WASTE_X100       [0x%04X] = %.0f\n", REG_INFER_WASTE_X100, snapshot->inference.comfort_waste * 100.0f);
    printf("  REG_INFER_STALE_X100       [0x%04X] = %.0f\n", REG_INFER_STALE_X100, snapshot->inference.stale_air_risk * 100.0f);
    printf("  REG_INFER_MAINT_X100       [0x%04X] = %.0f\n", REG_INFER_MAINT_X100, snapshot->inference.maintenance_priority * 100.0f);
    printf("  REG_INFER_COND_X100        [0x%04X] = %.0f\n", REG_INFER_COND_X100, snapshot->inference.condensation_risk * 100.0f);
    printf("  REG_INFER_INSTALL_X100     [0x%04X] = %.0f\n", REG_INFER_INSTALL_X100, snapshot->inference.install_quality * 100.0f);
    printf("  REG_ALERT_LEVEL            [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->inference.alert);
}

static void print_final_assessment(const vent_snapshot_t *snapshot)
{
    printf("\nFinal assessment\n");
    printf("  Device: %s\n", VENTLATTICE_DEVICE_NAME);
    printf("  Author: %s\n", VENTLATTICE_AUTHOR);
    printf("  Service score: %.1f\n", snapshot->inference.service_score);
    printf("  Maintenance priority: %.2f\n", snapshot->inference.maintenance_priority);
    printf("  Stale-air risk: %.2f\n", snapshot->inference.stale_air_risk);
    printf("  Comfort waste: %.2f\n", snapshot->inference.comfort_waste);
    printf("  Condensation risk: %.2f\n", snapshot->inference.condensation_risk);
    printf("  Primary reason: %s\n", inference_primary_reason(snapshot));
    printf("  Suggested action: %s\n",
           snapshot->airflow.blockage_index > 0.58f ?
           "Inspect vent face for obstruction and rebalance branch damper if needed." :
           (snapshot->inference.stale_air_risk > 0.55f ?
            "Increase delivered airflow or verify return path for occupied room." :
            "Continue monitoring; room delivery remains within learned limits."));
}

int main(void)
{
    vent_snapshot_t previous;
    vent_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[256];
    char report_packet[256];
    size_t used;
    uint32_t hour;
    const room_profile_t profile = ROOM_PROFILE_HOME_OFFICE;

    printf("%s firmware by %s\n", VENTLATTICE_DEVICE_NAME, VENTLATTICE_AUTHOR);
    printf("Firmware version: %s | profile=%s\n\n", VENTLATTICE_FIRMWARE_VERSION, profile_name(profile));

    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);

    for (hour = 1u; hour <= VENTLATTICE_LOOP_COUNT; ++hour) {
        current = previous;
        current.hour_index = hour;
        airflow_sample(&current.airflow, profile, hour);
        pressure_sample(&current.pressure, profile, hour, &current.airflow);
        environment_sample(&current.environment, profile, hour, &current.airflow);
        occupancy_sample(&current.occupancy, profile, hour, &current.environment);
        power_update(&current.power, &previous.power, &current);
        inference_update(&current.inference, &current, &previous);
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
