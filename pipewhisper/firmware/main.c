/*
 * PipeWhisper firmware simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/acoustic.h"
#include "drivers/flow.h"
#include "drivers/pressure.h"
#include "drivers/environment.h"
#include "drivers/power.h"
#include "drivers/inference.h"
#include "drivers/ble.h"
#include "drivers/logger.h"

static const char *profile_name(pipe_profile_t profile)
{
    switch (profile) {
    case PIPE_PROFILE_KITCHEN_COLD: return "kitchen-cold";
    case PIPE_PROFILE_LAUNDRY_HOT: return "laundry-hot";
    case PIPE_PROFILE_UTILITY_MIXED: return "utility-mixed";
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

static void snapshot_init(pipe_snapshot_t *snapshot, pipe_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    acoustic_init(&snapshot->acoustic, profile);
    pressure_init(&snapshot->pressure, profile);
    flow_init(&snapshot->flow, profile);
    environment_init(&snapshot->environment, profile);
    power_init(&snapshot->power, profile);
    inference_init(&snapshot->inference);
    snapshot->minute_index = 0u;
}

static void maybe_log_alert(const pipe_snapshot_t *current, const pipe_snapshot_t *previous, event_log_t *events)
{
    char detail[120];
    if (current->inference.alert == ALERT_NONE) return;
    if (current->inference.alert == previous->inference.alert && current->inference.maintenance_priority - previous->inference.maintenance_priority < 0.08f) return;
    snprintf(detail, sizeof(detail),
             "reason=%s leak=%.2f freeze=%.2f hammer=%.2f fixture=%s acoustic=%s",
             inference_primary_reason(current), current->inference.leak_confidence,
             current->inference.freeze_risk, current->pressure.hammer_score,
             flow_fixture_label(&current->flow), acoustic_event_label(&current->acoustic));
    if (current->inference.freeze_risk > 0.58f) logger_push_event(events, current->minute_index, current->inference.alert, "FREEZE", detail);
    else if (current->pressure.hammer_score > 0.55f) logger_push_event(events, current->minute_index, current->inference.alert, "HAMMER", detail);
    else logger_push_event(events, current->minute_index, current->inference.alert, "LEAK", detail);
}

static void print_snapshot(const pipe_snapshot_t *snapshot)
{
    printf("minute=%02u leak=%4.2f freeze=%4.2f hammer=%4.2f health=%5.1f install=%4.2f draw=%4.1fLPM temp=%4.1fC rh=%4.1f%% batt=%5.1f%% fixture=%s alert=%s\n",
           snapshot->minute_index, snapshot->inference.leak_confidence,
           snapshot->inference.freeze_risk, snapshot->pressure.hammer_score,
           snapshot->inference.health_index, snapshot->inference.install_quality,
           snapshot->flow.draw_estimate_lpm, snapshot->environment.surface_temp_c,
           snapshot->environment.humidity_rh, snapshot->power.battery_percent,
           flow_fixture_label(&snapshot->flow), alert_name(snapshot->inference.alert));
}

static void print_register_map(const pipe_snapshot_t *snapshot)
{
    printf("\nRegister snapshot for PipeWhisper by %s\n", PIPEWHISPER_AUTHOR);
    printf("  REG_PWR_STATUS           [0x%04X] = %.0f\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV       [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PERCENT  [0x%04X] = %.0f\n", REG_PWR_BATTERY_PERCENT, snapshot->power.battery_percent);
    printf("  REG_ACOUSTIC_RMS_X100    [0x%04X] = %.0f\n", REG_ACOUSTIC_RMS_X100, snapshot->acoustic.acoustic_rms * 100.0f);
    printf("  REG_ACOUSTIC_HZ          [0x%04X] = %.0f\n", REG_ACOUSTIC_HZ, snapshot->acoustic.dominant_hz);
    printf("  REG_ACOUSTIC_IMP_X100    [0x%04X] = %.0f\n", REG_ACOUSTIC_IMP_X100, snapshot->acoustic.impulsiveness * 100.0f);
    printf("  REG_ACOUSTIC_CHATTER     [0x%04X] = %.0f\n", REG_ACOUSTIC_CHATTER, snapshot->acoustic.chatter_index * 100.0f);
    printf("  REG_FLOW_LPM_X100        [0x%04X] = %.0f\n", REG_FLOW_LPM_X100, snapshot->flow.draw_estimate_lpm * 100.0f);
    printf("  REG_FLOW_DRIP_X100       [0x%04X] = %.0f\n", REG_FLOW_DRIP_X100, snapshot->flow.drip_confidence * 100.0f);
    printf("  REG_FLOW_STEADY_X100     [0x%04X] = %.0f\n", REG_FLOW_STEADY_X100, snapshot->flow.steady_flow_confidence * 100.0f);
    printf("  REG_FLOW_DRIFT_X100      [0x%04X] = %.0f\n", REG_FLOW_DRIFT_X100, snapshot->flow.signature_drift * 100.0f);
    printf("  REG_PRESSURE_HAMMER_X100 [0x%04X] = %.0f\n", REG_PRESSURE_HAMMER_X100, snapshot->pressure.hammer_score * 100.0f);
    printf("  REG_PRESSURE_IMPULSES    [0x%04X] = %.0f\n", REG_PRESSURE_IMPULSES, snapshot->pressure.impulse_count);
    printf("  REG_PRESSURE_RING_MS     [0x%04X] = %.0f\n", REG_PRESSURE_RING_MS, snapshot->pressure.ring_decay_ms);
    printf("  REG_PRESSURE_BURST_X100  [0x%04X] = %.0f\n", REG_PRESSURE_BURST_X100, snapshot->pressure.burst_risk * 100.0f);
    printf("  REG_ENV_SURFACE_X100     [0x%04X] = %.0f\n", REG_ENV_SURFACE_X100, snapshot->environment.surface_temp_c * 100.0f);
    printf("  REG_ENV_AMBIENT_X100     [0x%04X] = %.0f\n", REG_ENV_AMBIENT_X100, snapshot->environment.ambient_temp_c * 100.0f);
    printf("  REG_ENV_HUMIDITY_X100    [0x%04X] = %.0f\n", REG_ENV_HUMIDITY_X100, snapshot->environment.humidity_rh * 100.0f);
    printf("  REG_ENV_DEW_X100         [0x%04X] = %.0f\n", REG_ENV_DEW_X100, snapshot->environment.dew_risk * 100.0f);
    printf("  REG_ENV_FREEZE_X100      [0x%04X] = %.0f\n", REG_ENV_FREEZE_X100, snapshot->environment.freeze_slope_cph * 100.0f);
    printf("  REG_INFER_LEAK_X100      [0x%04X] = %.0f\n", REG_INFER_LEAK_X100, snapshot->inference.leak_confidence * 100.0f);
    printf("  REG_INFER_FREEZE_X100    [0x%04X] = %.0f\n", REG_INFER_FREEZE_X100, snapshot->inference.freeze_risk * 100.0f);
    printf("  REG_INFER_INSTALL_X100   [0x%04X] = %.0f\n", REG_INFER_INSTALL_X100, snapshot->inference.install_quality * 100.0f);
    printf("  REG_INFER_PRIORITY_X100  [0x%04X] = %.0f\n", REG_INFER_PRIORITY_X100, snapshot->inference.maintenance_priority * 100.0f);
    printf("  REG_INFER_HEALTH_X100    [0x%04X] = %.0f\n", REG_INFER_HEALTH_X100, snapshot->inference.health_index * 100.0f);
    printf("  REG_ALERT_LEVEL          [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->inference.alert);
}

static void print_final_assessment(const pipe_snapshot_t *snapshot)
{
    printf("\nFinal assessment\n");
    printf("  Device: %s\n", PIPEWHISPER_DEVICE_NAME);
    printf("  Author: %s\n", PIPEWHISPER_AUTHOR);
    printf("  Primary reason: %s\n", inference_primary_reason(snapshot));
    printf("  Leak confidence: %.2f\n", snapshot->inference.leak_confidence);
    printf("  Freeze risk: %.2f\n", snapshot->inference.freeze_risk);
    printf("  Hammer severity: %.2f\n", snapshot->pressure.hammer_score);
    printf("  Maintenance priority: %.2f\n", snapshot->inference.maintenance_priority);
    printf("  Suggested action: %s\n",
           snapshot->inference.freeze_risk > 0.55f ?
           "Inspect insulation and consider protective heating or flow automation." :
           (snapshot->inference.leak_confidence > 0.42f ?
            "Inspect downstream fixture for persistent drip or valve seepage." :
            "Continue monitoring; branch behavior remains within expected envelope."));
}

int main(void)
{
    pipe_snapshot_t previous;
    pipe_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[256];
    char report_packet[256];
    size_t used;
    uint32_t minute;
    const pipe_profile_t profile = PIPE_PROFILE_LAUNDRY_HOT;
    printf("%s firmware by %s\n", PIPEWHISPER_DEVICE_NAME, PIPEWHISPER_AUTHOR);
    printf("Firmware version: %s | profile=%s\n\n", PIPEWHISPER_FIRMWARE_VERSION, profile_name(profile));
    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);
    for (minute = 1u; minute <= PIPEWHISPER_LOOP_COUNT; ++minute) {
        current = previous;
        current.minute_index = minute;
        acoustic_sample(&current.acoustic, profile, minute);
        pressure_sample(&current.pressure, profile, minute, &current.acoustic);
        flow_update(&current.flow, &current.acoustic, &current.pressure, minute);
        environment_sample(&current.environment, profile, minute, &current.flow);
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
