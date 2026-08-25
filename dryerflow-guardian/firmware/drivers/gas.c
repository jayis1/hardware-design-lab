/*
 * gas.c — room-air contaminant trend modeling
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <math.h>
#include "gas.h"

static float g_voc = 9.0f;
static float g_nox = 4.0f;
static float g_co = 0.4f;

static float filt(float current, float sample, float alpha) {
    return current + alpha * (sample - current);
}

void gas_init(void) {
    g_voc = 9.0f;
    g_nox = 4.0f;
    g_co = 0.4f;
}

void gas_sample(dfg_sensor_frame_t *frame, uint32_t tick, bool gas_dryer) {
    const float t = (float)tick;
    const float active = frame->run_state == DFG_RUN_ACTIVE ? 1.0f : 0.0f;
    const float leak_event = (tick > 52U && tick < 62U) ? 1.0f : 0.0f;
    const float raw_voc = 7.0f + active * (4.0f + 1.0f * sinf(t / 9.0f)) + leak_event * 2.5f;
    const float raw_nox = 3.2f + active * 1.8f + leak_event * 1.7f;
    const float raw_co = gas_dryer ? 0.5f + active * (1.2f + 0.1f * sinf(t / 5.0f)) + leak_event * 8.0f : 0.0f;

    g_voc = filt(g_voc, raw_voc, DFG_GAS_FILTER_ALPHA);
    g_nox = filt(g_nox, raw_nox, DFG_GAS_FILTER_ALPHA);
    g_co = filt(g_co, raw_co, 0.16f);

    frame->voc_index = g_voc;
    frame->nox_index = g_nox;
    frame->co_ppm = g_co;
}
