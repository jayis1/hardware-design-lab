/*
 * moisture.c — StudGuard segmented capacitive moisture sensing
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "moisture.h"
#include "../registers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float seeded_noise(uint32_t tick, uint32_t idx) {
    uint32_t x = tick * 2654435761u + idx * 2246822519u + 0x9E3779B9u;
    x ^= x >> 15;
    x *= 0x85EBCA6Bu;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}

void moisture_init(moisture_state_t *state) {
    memset(state, 0, sizeof(*state));
    SG_CAP.CTRL = 1u;
    SG_CAP.CHANNEL_SELECT = 0x0Fu;
}

void moisture_sample(moisture_state_t *state, uint32_t tick, float humidity_rh, float thermal_bias) {
    static const float angles[CAP_SEGMENT_COUNT][2] = {
        { 0.0f,  1.0f},
        { 1.0f,  0.0f},
        { 0.0f, -1.0f},
        {-1.0f,  0.0f}
    };
    float sum = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    uint32_t i;
    float humidity_term = clampf((humidity_rh - 42.0f) / 55.0f, 0.0f, 1.0f);
    float leak_axis = 0.5f + 0.35f * sinf((float)tick * 0.00041f);

    for (i = 0; i < CAP_SEGMENT_COUNT; ++i) {
        float asym = (i == 0u ? leak_axis : (i == 2u ? 1.0f - leak_axis : 0.55f));
        float raw = 0.18f + humidity_term * 0.16f + thermal_bias * 0.05f + asym * 0.28f;
        raw += (seeded_noise(tick, i) - 0.5f) * 0.03f;
        state->segments[i] = clampf(raw, 0.0f, 1.0f);
        SG_CAP.RAW[i] = (uint32_t)(state->segments[i] * 4095.0f);
        sum += state->segments[i];
        vx += state->segments[i] * angles[i][0];
        vy += state->segments[i] * angles[i][1];
    }

    state->mean = sum / (float)CAP_SEGMENT_COUNT;
    state->vector_x = vx / (float)CAP_SEGMENT_COUNT;
    state->vector_y = vy / (float)CAP_SEGMENT_COUNT;
    state->delta = state->mean - state->baseline_mean;
    SG_CAP.STATUS = 1u;
}

void moisture_update_baseline(moisture_state_t *state, uint32_t baseline_count) {
    float n = (float)baseline_count;
    uint32_t i;
    if (baseline_count == 0u) {
        state->baseline_mean = state->mean;
        for (i = 0; i < CAP_SEGMENT_COUNT; ++i) {
            state->baseline_segments[i] = state->segments[i];
        }
        return;
    }

    state->baseline_mean = (state->baseline_mean * n + state->mean) / (n + 1.0f);
    for (i = 0; i < CAP_SEGMENT_COUNT; ++i) {
        state->baseline_segments[i] = (state->baseline_segments[i] * n + state->segments[i]) / (n + 1.0f);
    }
}

float moisture_score(const moisture_state_t *state, float *spread_score) {
    float top_rise = state->segments[0] - state->baseline_segments[0];
    float right_rise = state->segments[1] - state->baseline_segments[1];
    float bottom_rise = state->segments[2] - state->baseline_segments[2];
    float left_rise = state->segments[3] - state->baseline_segments[3];
    float directionality = fabsf(top_rise - bottom_rise) + fabsf(right_rise - left_rise);
    float leak = clampf(0.75f * state->delta + 0.55f * directionality, 0.0f, 1.0f);
    float spread = clampf(fabsf(state->vector_x) + fabsf(state->vector_y), 0.0f, 1.0f);

    if (spread_score != NULL) {
        *spread_score = spread;
    }
    return leak;
}

void moisture_direction_text(const moisture_state_t *state, char *buffer, uint32_t buffer_len) {
    const char *dir = "uniform";
    if (fabsf(state->vector_y) > fabsf(state->vector_x)) {
        dir = state->vector_y > 0.04f ? "source-above" : (state->vector_y < -0.04f ? "source-below" : "vertical-balanced");
    } else {
        dir = state->vector_x > 0.04f ? "source-right" : (state->vector_x < -0.04f ? "source-left" : "lateral-balanced");
    }
    snprintf(buffer, buffer_len, "%s mean=%.3f delta=%.3f vx=%.3f vy=%.3f",
             dir, state->mean, state->delta, state->vector_x, state->vector_y);
}
