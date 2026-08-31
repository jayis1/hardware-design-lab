/*
 * Threshold Veil acoustic driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "acoustic.h"

#include <math.h>

static float clampf(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

void acoustic_init(tv_acoustic_frame_t *frame)
{
    frame->low_band_db = 28.0f;
    frame->mid_band_db = 24.0f;
    frame->high_band_db = 20.0f;
    frame->transient_score = 0.1f;
    frame->impact_knock = false;
    frame->rolling_noise = false;
}

void acoustic_sample(tv_acoustic_frame_t *frame, const tv_env_frame_t *env, tv_mode_t mode, unsigned tick)
{
    const float hallway_cycle = sinf((float)tick * 0.21f);
    const float pressure_link = fabsf(env->pressure_pa) * 1.3f;
    const float door_bonus = env->door_closed ? 0.0f : 8.0f;
    const float quiet_bias = env->quiet_hours ? 4.0f : 0.0f;

    frame->impact_knock = (tick == 7 || tick == 24);
    frame->rolling_noise = (tick >= 19 && tick <= 22);

    frame->low_band_db = clampf(29.0f + 6.5f * hallway_cycle + pressure_link + door_bonus, 18.0f, 68.0f);
    frame->mid_band_db = clampf(24.0f + 5.0f * cosf((float)tick * 0.17f) + 0.7f * pressure_link + quiet_bias, 16.0f, 62.0f);
    frame->high_band_db = clampf(18.0f + 7.0f * sinf((float)tick * 0.31f) + 0.4f * pressure_link + (frame->rolling_noise ? 5.5f : 0.0f), 12.0f, 58.0f);

    frame->transient_score = 0.12f + (frame->impact_knock ? 0.65f : 0.0f) + (frame->rolling_noise ? 0.32f : 0.0f);
    if (mode == TV_MODE_QUIET) {
        frame->mid_band_db += 1.8f;
        frame->high_band_db += 1.1f;
        frame->transient_score += 0.08f;
    }
    if (!env->latch_aligned) {
        frame->mid_band_db += 2.0f;
        frame->high_band_db += 1.4f;
    }
}

float acoustic_leak_index(const tv_acoustic_frame_t *frame)
{
    const float weighted = 0.25f * frame->low_band_db +
                           0.45f * frame->mid_band_db +
                           0.30f * frame->high_band_db;
    const float normalized = (weighted - 20.0f) / 28.0f + frame->transient_score;
    return clampf(normalized, 0.0f, 4.0f);
}
