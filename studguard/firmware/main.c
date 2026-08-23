/*
 * main.c — StudGuard firmware simulation entry point
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "board.h"
#include "registers.h"
#include "drivers/acoustic.h"
#include "drivers/moisture.h"
#include "drivers/mesh.h"
#include "drivers/display.h"
#include "drivers/ble.h"
#include "drivers/power.h"
#include "drivers/logger.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sg_piezo_regs_t SG_PIEZO = {0};
sg_adc_regs_t SG_ADC = {0};
sg_capsense_regs_t SG_CAP = {0};
sg_uwb_regs_t SG_UWB = {0};
sg_ble_regs_t SG_BLE = {0};
sg_power_regs_t SG_PWR = {0};

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float dew_point_c(float temp_c, float rh) {
    const float a = 17.625f;
    const float b = 243.04f;
    float gamma = logf(rh / 100.0f) + (a * temp_c) / (b + temp_c);
    return (b * gamma) / (a - gamma);
}

static sg_event_t classify_event(const sg_measurement_t *m) {
    float condensation_gap = m->temperature_c - m->dew_point_c;
    if (m->confidence < 0.18f) {
        return EVENT_SENSOR_FAULT;
    }
    if (m->leak_activity > 0.80f && m->wetness_spread > 0.58f) {
        return EVENT_ACTIVE_PRESSURE_LEAK;
    }
    if (m->leak_activity > 0.58f && m->wetness_spread > 0.30f) {
        return EVENT_INTERMITTENT_LEAK;
    }
    if (condensation_gap < 1.0f && m->cap_delta < 0.12f && m->peer_attenuation < 0.18f) {
        return EVENT_CONDENSATION;
    }
    if (m->leak_activity < 0.30f && m->wetness_spread < 0.26f && m->cap_delta > 0.05f) {
        return EVENT_POST_REPAIR_DRYING;
    }
    return EVENT_NONE;
}

static const char *event_name(sg_event_t event) {
    switch (event) {
        case EVENT_NONE: return "none";
        case EVENT_CONDENSATION: return "condensation";
        case EVENT_INTERMITTENT_LEAK: return "intermittent-leak";
        case EVENT_ACTIVE_PRESSURE_LEAK: return "active-pressure-leak";
        case EVENT_POST_REPAIR_DRYING: return "post-repair-drying";
        case EVENT_SENSOR_FAULT: return "sensor-fault";
        default: return "unknown";
    }
}

static void update_mode(sg_device_status_t *status, const sg_measurement_t *m, uint32_t cycle_index) {
    if (cycle_index < 4u) {
        status->mode = MODE_BASELINE;
    } else if (m->leak_activity > 0.78f) {
        status->mode = MODE_DIAGNOSTIC;
    } else if (m->event == EVENT_POST_REPAIR_DRYING) {
        status->mode = MODE_REPAIR_VERIFICATION;
    } else {
        status->mode = MODE_MONITORING;
    }
}

static void populate_measurement(sg_measurement_t *m,
                                 const moisture_state_t *moisture,
                                 const acoustic_frame_t *frame,
                                 float acoustic_activity,
                                 float acoustic_confidence,
                                 uint32_t tick) {
    float daily = sinf((float)tick * 0.00005f);
    m->humidity_rh = clampf(47.0f + 18.0f * daily + moisture->mean * 22.0f, 28.0f, 95.0f);
    m->temperature_c = 21.0f + 2.8f * cosf((float)tick * 0.00004f);
    m->wall_temperature_c = m->temperature_c - 0.6f - moisture->mean * 0.8f;
    m->dew_point_c = dew_point_c(m->temperature_c, m->humidity_rh);
    memcpy(m->cap_segments, moisture->segments, sizeof(m->cap_segments));
    m->cap_vector_x = moisture->vector_x;
    m->cap_vector_y = moisture->vector_y;
    m->cap_mean = moisture->mean;
    m->cap_delta = moisture->delta;
    m->acoustic_energy = frame->energy;
    m->acoustic_decay_ms = frame->decay_ms;
    m->spectral_centroid_hz = frame->centroid_hz;
    m->phase_stability = frame->phase_stability;
    m->damping_ratio = frame->damping_ratio;
    m->leak_activity = clampf(0.48f * acoustic_activity + 0.44f * clampf(moisture->delta * 1.8f, 0.0f, 1.0f) + 0.08f * fabsf(moisture->vector_y), 0.0f, 1.0f);
    m->wetness_spread = clampf(fabsf(moisture->vector_x) + fabsf(moisture->vector_y) + m->cap_mean * 0.35f, 0.0f, 1.0f);
    m->confidence = clampf(0.52f * acoustic_confidence + 0.30f * (1.0f - fabsf(moisture->vector_x - moisture->vector_y)) + 0.18f * (1.0f - fabsf(frame->damping_ratio - 0.35f)), 0.0f, 1.0f);
    m->origin_band = 0.0f;
    m->peer_attenuation = 0.0f;
    m->peer_count = 0u;
    m->event = EVENT_NONE;
}

static void print_cycle_header(uint32_t cycle, uint32_t tick) {
    printf("\n=== StudGuard cycle %u @ tick %u ms ===\n", cycle, tick);
}

int main(void) {
    sg_device_status_t status;
    power_state_t power;
    moisture_state_t moisture;
    acoustic_frame_t acoustic;
    acoustic_baseline_t acoustic_baseline = {0};
    mesh_state_t mesh;
    log_state_t logs;
    uint32_t cycle;
    uint32_t tick = 0u;
    uint32_t baseline_count = 0u;

    memset(&status, 0, sizeof(status));
    status.node_id = 3u;
    status.mode = MODE_BASELINE;
    status.mesh_enabled = 1u;
    status.mounted = 1u;
    status.interval_ms = 60000u;

    power_init(&power);
    moisture_init(&moisture);
    acoustic_init();
    mesh_init(&mesh, status.node_id);
    logger_init(&logs);
    display_init();
    ble_init();
    display_render_banner("StudGuard booted — author jayis1");

    for (cycle = 0u; cycle < 16u; ++cycle) {
        sg_measurement_t measurement;
        char acoustic_text[160];
        char moisture_text[160];
        char mesh_text[160];
        char ble_text[512];
        char peer_text[512];

        tick += status.interval_ms;
        print_cycle_header(cycle, tick);

        moisture_sample(&moisture, tick, 52.0f + 4.0f * sinf((float)tick * 0.00008f), 0.2f + 0.3f * sinf((float)tick * 0.00013f));
        acoustic_capture_frame(&acoustic, tick, clampf(moisture.mean + moisture.delta * 2.0f, 0.0f, 1.0f), 20.5f + 2.0f * cosf((float)tick * 0.00002f));

        if (cycle < 4u) {
            moisture_update_baseline(&moisture, baseline_count);
            acoustic_update_baseline(&acoustic_baseline, &acoustic, baseline_count);
            baseline_count++;
        }

        {
            float acoustic_confidence = 0.0f;
            float acoustic_activity = acoustic_compare_to_baseline(&acoustic_baseline, &acoustic, &acoustic_confidence);
            populate_measurement(&measurement, &moisture, &acoustic, acoustic_activity, acoustic_confidence, tick);
            mesh_synthesize_peers(&mesh, tick, measurement.leak_activity, measurement.wetness_spread, measurement.confidence);
            mesh_integrate_measurement(&mesh, &measurement);
            measurement.event = classify_event(&measurement);
            update_mode(&status, &measurement, cycle);
            power_tick(&power, status.mode, measurement.leak_activity, measurement.confidence, status.interval_ms);

            status.battery_percent = power.battery_percent;
            status.interval_ms = power.interval_ms;
            status.uptime_s = tick / 1000u;

            logger_append(&logs, tick, &status, &measurement);
            acoustic_debug_summary(&acoustic, acoustic_text, sizeof(acoustic_text));
            moisture_direction_text(&moisture, moisture_text, sizeof(moisture_text));
            mesh_summary(&mesh, mesh_text, sizeof(mesh_text));
            ble_encode_status(&status, &measurement, ble_text, sizeof(ble_text));
            ble_encode_peers(mesh.peers, mesh.count, peer_text, sizeof(peer_text));

            display_render_status(&status, &measurement);
            printf("[acoustic] %s\n", acoustic_text);
            printf("[moisture] %s\n", moisture_text);
            printf("[mesh] %s\n", mesh_text);
            printf("[ble] %s\n", ble_text);
            printf("[ble-peers] %s\n", peer_text);
            printf("[event] %s\n", event_name(measurement.event));
        }
    }

    {
        char export_text[2048];
        logger_export_latest(&logs, export_text, sizeof(export_text), 8u);
        printf("\n=== Recent StudGuard log export ===\n%s", export_text);
    }

    return 0;
}
