/*
 * SplintSense simulation firmware
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/power.h"
#include "drivers/pressure.h"
#include "drivers/moisture.h"
#include "drivers/sensor_hub.h"
#include "drivers/ble.h"
#include "drivers/logger.h"
#include "drivers/haptics.h"

static const char *profile_name(splint_profile_t profile)
{
    return profile == SPLINT_PROFILE_ANKLE ? "ankle" : "wrist";
}

static const char *alert_name(alert_level_t level)
{
    switch (level) {
    case ALERT_NONE:
        return "none";
    case ALERT_INFO:
        return "info";
    case ALERT_CAUTION:
        return "caution";
    case ALERT_WARNING:
        return "warning";
    case ALERT_CRITICAL:
        return "critical";
    default:
        return "unknown";
    }
}

static float clampf(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float compute_motion_factor(const env_frame_t *env)
{
    return clampf((env->acceleration_g - 0.05f) / 0.85f, 0.0f, 1.0f);
}

static void snapshot_init(recovery_snapshot_t *snapshot, splint_profile_t profile)
{
    memset(snapshot, 0, sizeof(*snapshot));
    sensor_hub_init(&snapshot->env, profile);
    pressure_init(&snapshot->pressure, profile);
    moisture_init(&snapshot->moisture);
    power_init(&snapshot->power, profile);
    snapshot->recovery_stability_index = 96.0f;
    snapshot->fit_score = 95.0f;
    snapshot->odor_risk = 10.0f;
    snapshot->comfort_score = 94.0f;
    snapshot->compliance_score = 97.0f;
    snapshot->alert = ALERT_NONE;
    snapshot->minute_index = 0u;
}

static void print_register_map(const recovery_snapshot_t *snapshot)
{
    size_t i;
    printf("Register snapshot at minute %u\n", snapshot->minute_index);
    printf("  REG_PWR_STATUS        [0x%04X] = %.0f\n", REG_PWR_STATUS, power_status_register(&snapshot->power));
    printf("  REG_PWR_BATTERY_MV    [0x%04X] = %.0f\n", REG_PWR_BATTERY_MV, snapshot->power.battery_mv);
    printf("  REG_PWR_BATTERY_PCT   [0x%04X] = %.0f\n", REG_PWR_BATTERY_PCT, snapshot->power.battery_percent);
    printf("  REG_ENV_TEMP_C_X100   [0x%04X] = %.0f\n", REG_ENV_TEMP_C_X100, snapshot->env.temperature_c * 100.0f);
    printf("  REG_ENV_HUMIDITY_X100 [0x%04X] = %.0f\n", REG_ENV_HUMIDITY_X100, snapshot->env.humidity_rh * 100.0f);
    printf("  REG_ENV_VOC_INDEX     [0x%04X] = %.0f\n", REG_ENV_VOC_INDEX, snapshot->env.voc_index);
    printf("  REG_ENV_IMPACT_X100   [0x%04X] = %.0f\n", REG_ENV_IMPACT_X100, snapshot->env.impact_g * 100.0f);
    printf("  REG_ENV_STEPS         [0x%04X] = %u\n", REG_ENV_STEPS, snapshot->env.step_count);
    for (i = 0u; i < SPLINTSENSE_PRESSURE_ZONES; ++i) {
        printf("  REG_PRESSURE_%zu       [0x%04X] = %.0f\n", i, REG_PRESSURE_BASE + (unsigned)i, snapshot->pressure.zones[i] * 100.0f);
    }
    for (i = 0u; i < SPLINTSENSE_MOISTURE_ZONES; ++i) {
        printf("  REG_MOISTURE_%zu       [0x%04X] = %.0f\n", i, REG_MOISTURE_BASE + (unsigned)i, snapshot->moisture.zones[i] * 100.0f);
    }
    printf("  REG_RSI_X100          [0x%04X] = %.0f\n", REG_RSI_X100, snapshot->recovery_stability_index * 100.0f);
    printf("  REG_FIT_X100          [0x%04X] = %.0f\n", REG_FIT_X100, snapshot->fit_score * 100.0f);
    printf("  REG_ODOR_X100         [0x%04X] = %.0f\n", REG_ODOR_X100, snapshot->odor_risk * 100.0f);
    printf("  REG_COMPLIANCE_X100   [0x%04X] = %.0f\n", REG_COMPLIANCE_X100, snapshot->compliance_score * 100.0f);
    printf("  REG_ALERT_LEVEL       [0x%04X] = %u\n", REG_ALERT_LEVEL, (unsigned)snapshot->alert);
}

static void maybe_log_alert(const recovery_snapshot_t *current,
                            const recovery_snapshot_t *previous,
                            event_log_t *events)
{
    char detail[96];
    if (current->alert == ALERT_NONE) {
        return;
    }
    if (current->alert == previous->alert && fabsf(current->recovery_stability_index - previous->recovery_stability_index) < 4.0f) {
        return;
    }

    snprintf(detail,
             sizeof(detail),
             "fit=%.1f moisture=%.1f voc=%.1f impact=%.2f haptic=%s",
             current->fit_score,
             current->moisture.average,
             current->env.voc_index,
             current->env.impact_g,
             haptics_pattern_for_alert(current->alert, current->moisture.persistence_minutes));

    if (current->env.impact_g > 2.2f) {
        logger_push_event(events, current->minute_index, current->alert, "IMPACT", detail);
    } else if (current->moisture.persistence_minutes > 18.0f) {
        logger_push_event(events, current->minute_index, current->alert, "MOISTURE", detail);
    } else if (current->fit_score < 55.0f) {
        logger_push_event(events, current->minute_index, current->alert, "FIT", detail);
    } else {
        logger_push_event(events, current->minute_index, current->alert, "MICROCLIMATE", detail);
    }
}

static void print_snapshot(const recovery_snapshot_t *snapshot)
{
    printf(
        "minute=%02u rsi=%5.1f fit=%5.1f comfort=%5.1f compliance=%5.1f battery=%5.1f%% temp=%4.1fC humidity=%4.1f%% voc=%5.1f pressurePeak=%5.1f moistureAvg=%5.1f impact=%4.2fg alert=%s\n",
        snapshot->minute_index,
        snapshot->recovery_stability_index,
        snapshot->fit_score,
        snapshot->comfort_score,
        snapshot->compliance_score,
        snapshot->power.battery_percent,
        snapshot->env.temperature_c,
        snapshot->env.humidity_rh,
        snapshot->env.voc_index,
        snapshot->pressure.max_zone,
        snapshot->moisture.average,
        snapshot->env.impact_g,
        alert_name(snapshot->alert)
    );
}

static void print_final_assessment(const snapshot_log_t *snapshots, splint_profile_t profile)
{
    const recovery_snapshot_t *latest = &snapshots->history[snapshots->count - 1u];
    printf("\nFinal assessment for %s profile:\n", profile_name(profile));
    printf("  Recovery Stability Index: %.1f\n", latest->recovery_stability_index);
    printf("  Fit score: %.1f\n", latest->fit_score);
    printf("  Moisture persistence: %.1f min\n", latest->moisture.persistence_minutes);
    printf("  Suggested action: %s\n",
           latest->alert >= ALERT_WARNING ?
           "Check brace fit and inspect liner at first opportunity." :
           "Continue observation; no urgent intervention.");
}

int main(void)
{
    recovery_snapshot_t previous;
    recovery_snapshot_t current;
    event_log_t events;
    snapshot_log_t snapshots;
    char status_packet[320];
    char clinician_packet[SPLINTSENSE_EXPORT_TEXT_CAPACITY];
    size_t used;
    uint32_t minute;
    const splint_profile_t profile = SPLINT_PROFILE_ANKLE;

    printf("SplintSense simulation firmware by %s\n", SPLINTSENSE_AUTHOR);
    printf("Device: %s | Firmware: %s | Profile: %s\n\n",
           SPLINTSENSE_DEVICE_NAME,
           SPLINTSENSE_FIRMWARE_VERSION,
           profile_name(profile));

    snapshot_init(&previous, profile);
    logger_init(&events, &snapshots);
    logger_push_snapshot(&snapshots, &previous);
    print_snapshot(&previous);

    for (minute = 1u; minute <= SPLINTSENSE_LOOP_COUNT; ++minute) {
        float motion_factor;
        current = previous;
        current.minute_index = minute;

        sensor_hub_sample(&current.env, profile, minute);
        motion_factor = compute_motion_factor(&current.env);
        pressure_sample(&current.pressure, profile, minute, motion_factor);
        moisture_sample(&current.moisture, minute, current.env.humidity_rh, motion_factor);
        power_update(&current.power, &previous, motion_factor, minute);
        sensor_hub_fuse(&current, &previous);

        maybe_log_alert(&current, &previous, &events);
        logger_push_snapshot(&snapshots, &current);
        print_snapshot(&current);
        previous = current;
    }

    used = ble_build_status_packet(&previous, status_packet, sizeof(status_packet));
    printf("\nBLE status packet (%zu bytes):\n%s\n", used, status_packet);

    used = ble_build_clinician_packet(&previous, events.alerts, events.count, clinician_packet, sizeof(clinician_packet));
    printf("\nBLE clinician packet (%zu bytes):\n%s\n", used, clinician_packet);

    printf("\n");
    print_register_map(&previous);
    logger_print_summary(&events, &snapshots);
    print_final_assessment(&snapshots, profile);
    return 0;
}
