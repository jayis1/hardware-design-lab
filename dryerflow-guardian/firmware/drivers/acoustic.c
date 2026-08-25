/*
 * acoustic.c — blower and turbulence feature extraction model
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "acoustic.h"

static float g_turbulence = 2.5f;
static float g_energy = 22.0f;

static float lp(float x, float y, float alpha) {
    return x + alpha * (y - x);
}

void acoustic_init(void) {
    g_turbulence = 2.5f;
    g_energy = 22.0f;
}

void acoustic_sample(dfg_sensor_frame_t *frame, uint32_t tick) {
    const float t = (float)tick;
    const float active = frame->run_state == DFG_RUN_ACTIVE ? 1.0f : 0.0f;
    const float drydown = frame->run_state == DFG_RUN_DRYDOWN ? 1.0f : 0.0f;
    const float blade_band = active * (18.0f + 3.0f * sinf(t / 3.8f));
    const float hiss_band = active * (8.0f + 4.0f * cosf(t / 5.1f));
    const float blockage_band = active * (0.08f * t) + drydown * 1.5f;
    const float raw_energy = 12.0f + blade_band + hiss_band;
    const float raw_turbulence = 1.2f + 0.35f * hiss_band + 0.20f * blockage_band;

    g_energy = lp(g_energy, raw_energy, 0.20f);
    g_turbulence = lp(g_turbulence, raw_turbulence, DFG_ACOUSTIC_FILTER_ALPHA);

    frame->blower_energy = g_energy;
    frame->turbulence_score = g_turbulence;
}
