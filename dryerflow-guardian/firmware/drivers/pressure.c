/*
 * pressure.c — differential pressure model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "pressure.h"

static float g_filtered_pressure = 6.0f;
static float g_filtered_static = 1.8f;

static float pressure_low_pass(float prev, float sample, float alpha) {
    return prev + alpha * (sample - prev);
}

void pressure_init(void) {
    g_filtered_pressure = 6.0f;
    g_filtered_static = 1.8f;
}

void pressure_sample(dfg_sensor_frame_t *frame, uint32_t tick) {
    const float cycle = (float)tick;
    const float startup = 7.0f * sinf(cycle / 8.0f);
    const float turbulence = 3.0f * sinf(cycle / 2.7f);
    const float lint_bias = 0.035f * (float)(tick > 40 ? tick - 40 : 0);
    const float active_boost = frame->run_state == DFG_RUN_ACTIVE ? 14.0f : 2.0f;
    const float drydown_drop = frame->run_state == DFG_RUN_DRYDOWN ? -4.0f : 0.0f;
    const float raw_pressure = active_boost + startup + turbulence + lint_bias + drydown_drop;
    const float raw_static = 1.2f + 0.4f * cosf(cycle / 11.0f) + 0.08f * raw_pressure;

    g_filtered_pressure = pressure_low_pass(g_filtered_pressure, raw_pressure, DFG_PRESSURE_FILTER_ALPHA);
    g_filtered_static = pressure_low_pass(g_filtered_static, raw_static, 0.18f);

    if (g_filtered_pressure < 0.0f) {
        g_filtered_pressure = 0.0f;
    }
    if (g_filtered_static < 0.0f) {
        g_filtered_static = 0.0f;
    }

    frame->pressure_pa = g_filtered_pressure;
    frame->static_pressure_pa = g_filtered_static;
}
