/*
 * DrainVeil firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/flow.h"
#include "drivers/pressure.h"
#include "drivers/chemistry.h"
#include "drivers/thermal.h"
#include "drivers/power.h"
#include "drivers/inference.h"
#include "drivers/ble.h"
#include "drivers/logger.h"

float dv_clampf(float value, float lo, float hi)
{
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}

float dv_lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float dv_profile_bias(install_profile_t profile, float sink, float floor, float interceptor)
{
    switch (profile) {
    case INSTALL_PROFILE_KITCHEN_SINK: return sink;
    case INSTALL_PROFILE_FLOOR_DRAIN: return floor;
    case INSTALL_PROFILE_GREASE_INTERCEPTOR: return interceptor;
    default: return sink;
    }
}

const char *dv_profile_name(install_profile_t profile)
{
    switch (profile) {
    case INSTALL_PROFILE_KITCHEN_SINK: return "kitchen-sink";
    case INSTALL_PROFILE_FLOOR_DRAIN: return "floor-drain";
    case INSTALL_PROFILE_GREASE_INTERCEPTOR: return "grease-interceptor";
    default: return "unknown";
    }
}

const char *dv_alert_name(alert_level_t level)
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

static void snapshot_init(drain_snapshot_t *snapshot, install_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    flow_init(&snapshot->flow, profile);
    pressure_init(&snapshot->pressure, profile);
    chemistry_init(&snapshot->chemistry, profile);
    thermal_init(&snapshot->thermal, profile);
    power_init(&snapshot->power, profile);
    inference_init(&snapshot->inference);
    snapshot->minute_index = 0u;
}

static void maybe_log_alert(const drain_snapshot_t *current, const drain_snapshot_t *previous, event_log_t *events)
{
    char detail[120];
    if (current->inference.alert == ALERT_NONE) return;
    if (current->inference.alert == previous->inference.alert &&
        current->inference.maintenance_priority - previous->inference.maintenance_priority < 0.04f) {
        return;
    }
    snprintf(detail, sizeof(detail),
             "reason=%s flow=%s pressure=%s chemistry=%s thermal=%s fill=%.1f drain=%.1fs gas=%.2f",
             inference_primary_reason(current),
             flow_pattern_label(&current->flow),
             pressure_pattern_label(&current->pressure),
             chemistry_status_label(&current->chemistry),
             thermal_status_label(&current->thermal),
             current->flow.fill_height_percent,
             current->flow.drain_time_s,
             current->chemistry.h2s_ppm);
    if (current->inference.freeze_risk > 0.72f) {
        logger_push_event(events, current->minute_index, current->inference.alert, "FREEZE", detail);
    } else if (current->inference.clog_risk > 0.70f) {
        logger_push_event(events, current->minute_index, current->inference.alert, "CLOG", detail);
    } else {
        logger_push_event(events, current->minute_index, current->inference.alert, "ODOR", detail);
    }
}

static void print_snapshot(const drain_snapshot_t *snapshot)
{
    printf("minute=%02u fill=%5.1f%% flow=%5.2fL/m drain=%5.1fs pressure=%4.1fkPa gas=%4.2fppm freeze_margin=%5.2fC clog=%4.2f odor=%4.2f freeze=%4.2f alert=%s\n",
           snapshot->minute_index,
           snapshot->flow.fill_height_percent,
           snapshot->flow.flow_lpm,
           snapshot->flow.drain_time_s,
           snapshot->pressure.line_pressure_kpa,
           snapshot->chemistry.h2s_ppm,
           snapshot->thermal.freeze_margin_c,
           snapshot->inference.clog_risk,
           snapshot->inference.odor_risk,
           snapshot->inference.freeze_risk,
           dv_alert_name(snapshot->inference.alert));
}

static void print_register_map(const drain_snapshot_t *snapshot)
{
    printf("\nRegister snapshot for DrainVeil by %s\n", DRAINVEIL_AUTHOR);
    printf("  REG_PWR_STATUS            [0x%04X] = %.0f\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV        [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PERCENT   [0x%04X] = %.0f\n", REG_PWR_BATTERY_PERCENT, snapshot->power.battery_percent);
    printf("  REG_PWR_EST_DAYS          [0x%04X] = %.0f\n", REG_PWR_EST_DAYS, snapshot->power.estimated_days_left);
    printf("  REG_PWR_RAIL_NOISE        [0x%04X] = %.0f\n", REG_PWR_RAIL_NOISE, snapshot->power.rail_noise_mv);
    printf("  REG_FLOW_VELOCITY_X100    [0x%04X] = %.0f\n", REG_FLOW_VELOCITY_X100, snapshot->flow.ultrasonic_velocity * 100.0f);
    printf("  REG_FLOW_REFLECT_X100     [0x%04X] = %.0f\n", REG_FLOW_REFLECT_X100, snapshot->flow.reflection_strength * 100.0f);
    printf("  REG_FLOW_TURB_X100        [0x%04X] = %.0f\n", REG_FLOW_TURB_X100, snapshot->flow.turbulence_index * 100.0f);
    printf("  REG_FLOW_FILL_X100        [0x%04X] = %.0f\n", REG_FLOW_FILL_X100, snapshot->flow.fill_height_percent * 100.0f);
    printf("  REG_FLOW_SLUG_X100        [0x%04X] = %.0f\n", REG_FLOW_SLUG_X100, snapshot->flow.slug_probability * 100.0f);
    printf("  REG_FLOW_LPM_X100         [0x%04X] = %.0f\n", REG_FLOW_LPM_X100, snapshot->flow.flow_lpm * 100.0f);
    printf("  REG_FLOW_DRAIN_TIME_X10   [0x%04X] = %.0f\n", REG_FLOW_DRAIN_TIME_X10, snapshot->flow.drain_time_s * 10.0f);
    printf("  REG_FLOW_BUBBLE_X100      [0x%04X] = %.0f\n", REG_FLOW_BUBBLE_X100, snapshot->flow.bubble_factor * 100.0f);
    printf("  REG_PRESSURE_KPA_X100     [0x%04X] = %.0f\n", REG_PRESSURE_KPA_X100, snapshot->pressure.line_pressure_kpa * 100.0f);
    printf("  REG_PRESSURE_PULSE_X100   [0x%04X] = %.0f\n", REG_PRESSURE_PULSE_X100, snapshot->pressure.pulse_variance * 100.0f);
    printf("  REG_PRESSURE_HAMMER_X100  [0x%04X] = %.0f\n", REG_PRESSURE_HAMMER_X100, snapshot->pressure.water_hammer_score * 100.0f);
    printf("  REG_PRESSURE_TRAP_X100    [0x%04X] = %.0f\n", REG_PRESSURE_TRAP_X100, snapshot->pressure.trap_oscillation * 100.0f);
    printf("  REG_PRESSURE_VIBE_X100    [0x%04X] = %.0f\n", REG_PRESSURE_VIBE_X100, snapshot->pressure.vibration_rms * 100.0f);
    printf("  REG_PRESSURE_BLOCK_X100   [0x%04X] = %.0f\n", REG_PRESSURE_BLOCK_X100, snapshot->pressure.blockage_gradient * 100.0f);
    printf("  REG_PRESSURE_BRANCH_X100  [0x%04X] = %.0f\n", REG_PRESSURE_BRANCH_X100, snapshot->pressure.branch_asymmetry * 100.0f);
    printf("  REG_CHEM_RH_X100          [0x%04X] = %.0f\n", REG_CHEM_RH_X100, snapshot->chemistry.humidity_percent * 100.0f);
    printf("  REG_CHEM_COND_X100        [0x%04X] = %.0f\n", REG_CHEM_COND_X100, snapshot->chemistry.condensate_risk * 100.0f);
    printf("  REG_CHEM_H2S_X100         [0x%04X] = %.0f\n", REG_CHEM_H2S_X100, snapshot->chemistry.h2s_ppm * 100.0f);
    printf("  REG_CHEM_VOC_X100         [0x%04X] = %.0f\n", REG_CHEM_VOC_X100, snapshot->chemistry.voc_index * 100.0f);
    printf("  REG_CHEM_BIO_X100         [0x%04X] = %.0f\n", REG_CHEM_BIO_X100, snapshot->chemistry.biofilm_proxy * 100.0f);
    printf("  REG_CHEM_GREASE_X100      [0x%04X] = %.0f\n", REG_CHEM_GREASE_X100, snapshot->chemistry.grease_proxy * 100.0f);
    printf("  REG_CHEM_CORROSION_X100   [0x%04X] = %.0f\n", REG_CHEM_CORROSION_X100, snapshot->chemistry.corrosion_index * 100.0f);
    printf("  REG_THERM_PIPE_X100       [0x%04X] = %.0f\n", REG_THERM_PIPE_X100, snapshot->thermal.pipe_temp_c * 100.0f);
    printf("  REG_THERM_AMBIENT_X100    [0x%04X] = %.0f\n", REG_THERM_AMBIENT_X100, snapshot->thermal.ambient_temp_c * 100.0f);
    printf("  REG_THERM_FREEZE_X100     [0x%04X] = %.0f\n", REG_THERM_FREEZE_X100, snapshot->thermal.freeze_margin_c * 100.0f);
    printf("  REG_THERM_RECOVERY_X10    [0x%04X] = %.0f\n", REG_THERM_RECOVERY_X10, snapshot->thermal.thermal_recovery_s * 10.0f);
    printf("  REG_THERM_HEATLEAK_X100   [0x%04X] = %.0f\n", REG_THERM_HEATLEAK_X100, snapshot->thermal.heat_leak_score * 100.0f);
    printf("  REG_THERM_COLDSLUG_X100   [0x%04X] = %.0f\n", REG_THERM_COLDSLUG_X100, snapshot->thermal.cold_slug_index * 100.0f);
    printf("  REG_INFER_CLOG_X100       [0x%04X] = %.0f\n", REG_INFER_CLOG_X100, snapshot->inference.clog_risk * 100.0f);
    printf("  REG_INFER_ODOR_X100       [0x%04X] = %.0f\n", REG_INFER_ODOR_X100, snapshot->inference.odor_risk * 100.0f);
    printf("  REG_INFER_FREEZE_X100     [0x%04X] = %.0f\n", REG_INFER_FREEZE_X100, snapshot->inference.freeze_risk * 100.0f);
    printf("  REG_INFER_PRIORITY_X100   [0x%04X] = %.0f\n", REG_INFER_PRIORITY_X100, snapshot->inference.maintenance_priority * 100.0f);
    printf("  REG_INFER_SERVICE_X100    [0x%04X] = %.0f\n", REG_INFER_SERVICE_X100, snapshot->inference.service_score * 100.0f);
    printf("  REG_INFER_CONFIDENCE_X100 [0x%04X] = %.0f\n", REG_INFER_CONFIDENCE_X100, snapshot->inference.confidence * 100.0f);
    printf("  REG_INFER_EFFICIENCY_X100 [0x%04X] = %.0f\n", REG_INFER_EFFICIENCY_X100, snapshot->inference.efficiency_penalty * 100.0f);
    printf("  REG_ALERT_LEVEL           [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->inference.alert);
}

static void print_final_assessment(const drain_snapshot_t *snapshot, install_profile_t profile)
{
    const char *suggested;
    if (snapshot->inference.freeze_risk > 0.72f) {
        suggested = "Insulate the pipe run, add trap-primer activity, and correct cold-air exposure before a freeze rupture occurs.";
    } else if (snapshot->inference.clog_risk > 0.74f) {
        suggested = "Schedule a hydro-jet or mechanical cleanout and inspect the branch transition where fill height is accumulating.";
    } else if (snapshot->inference.odor_risk > 0.64f) {
        suggested = "Flush the line, clean the trap, and inspect vent balance because biofilm and gas production are rising.";
    } else {
        suggested = "Continue monitoring; drain behavior is currently within the modeled healthy envelope.";
    }

    printf("\nFinal assessment\n");
    printf("  Device: %s\n", DRAINVEIL_DEVICE_NAME);
    printf("  Author: %s\n", DRAINVEIL_AUTHOR);
    printf("  Profile: %s\n", dv_profile_name(profile));
    printf("  Primary reason: %s\n", inference_primary_reason(snapshot));
    printf("  Clog risk: %.2f\n", snapshot->inference.clog_risk);
    printf("  Odor risk: %.2f\n", snapshot->inference.odor_risk);
    printf("  Freeze risk: %.2f\n", snapshot->inference.freeze_risk);
    printf("  Maintenance priority: %.2f\n", snapshot->inference.maintenance_priority);
    printf("  Service score: %.2f\n", snapshot->inference.service_score);
    printf("  Suggested action: %s\n", suggested);
}

int main(void)
{
    drain_snapshot_t previous;
    drain_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[DRAINVEIL_MAX_PACKET];
    char report_packet[DRAINVEIL_MAX_PACKET];
    size_t used;
    uint32_t minute;
    const install_profile_t profile = INSTALL_PROFILE_GREASE_INTERCEPTOR;

    printf("%s firmware by %s\n", DRAINVEIL_DEVICE_NAME, DRAINVEIL_AUTHOR);
    printf("Firmware version: %s | profile=%s\n\n", DRAINVEIL_FIRMWARE_VERSION, dv_profile_name(profile));

    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);

    for (minute = 1u; minute <= DRAINVEIL_LOOP_COUNT; ++minute) {
        current = previous;
        current.minute_index = minute;
        flow_sample(&current.flow, profile, minute);
        pressure_sample(&current.pressure, profile, minute, &current.flow);
        chemistry_sample(&current.chemistry, profile, minute, &current.flow, &current.pressure);
        thermal_sample(&current.thermal, profile, minute, &current.flow, &current.chemistry);
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
