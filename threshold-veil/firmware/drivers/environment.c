/*
 * Threshold Veil environment driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "environment.h"

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

static float event_pulse(unsigned tick, unsigned start, unsigned width, float amplitude)
{
    if (tick < start || tick > start + width) {
        return 0.0f;
    }
    const float x = (float)(tick - start) / (float)(width == 0 ? 1 : width);
    return amplitude * sinf(x * 3.1415926f);
}

void env_init(tv_env_frame_t *frame)
{
    frame->indoor_temp_c = 22.8f;
    frame->indoor_humidity_pct = 43.0f;
    frame->indoor_voc_index = 56.0f;
    frame->corridor_temp_c = 24.1f;
    frame->corridor_humidity_pct = 39.0f;
    frame->corridor_voc_index = 62.0f;
    frame->corridor_pm25_ugm3 = 7.0f;
    frame->corridor_pm10_ugm3 = 12.0f;
    frame->pressure_pa = -0.4f;
    frame->threshold_temp_c = 21.2f;
    frame->door_closed = true;
    frame->latch_aligned = true;
    frame->quiet_hours = false;
    frame->tick = 0;
}

void env_sample(tv_env_frame_t *frame, tv_mode_t mode, unsigned tick)
{
    const float daily = sinf((float)tick * 0.18f);
    const float hallway = cosf((float)tick * 0.11f);
    const float smoke_pulse = event_pulse(tick, 10, 6, 48.0f);
    const float odor_pulse = event_pulse(tick, 18, 5, 36.0f);
    const float pressure_pulse = event_pulse(tick, 26, 4, 7.5f);
    const float winter_draft = event_pulse(tick, 32, 6, 3.3f);

    frame->tick = tick;
    frame->quiet_hours = (tick >= 18 && tick <= 30);
    frame->door_closed = !(tick == 8 || tick == 9 || tick == 23);
    frame->latch_aligned = (tick % 17 != 0);

    frame->indoor_temp_c = 22.4f + 0.8f * daily - 0.2f * winter_draft;
    frame->indoor_humidity_pct = clampf(44.0f + 5.5f * sinf((float)tick * 0.07f), 31.0f, 60.0f);
    frame->indoor_voc_index = clampf(54.0f + 6.0f * daily + 0.25f * odor_pulse, 25.0f, 210.0f);

    frame->corridor_temp_c = 23.5f + 1.6f * hallway - 0.4f * winter_draft;
    frame->corridor_humidity_pct = clampf(38.0f + 3.0f * cosf((float)tick * 0.05f), 24.0f, 55.0f);
    frame->corridor_voc_index = clampf(60.0f + 5.0f * hallway + odor_pulse + 0.4f * smoke_pulse, 22.0f, 320.0f);
    frame->corridor_pm25_ugm3 = clampf(8.0f + 2.0f * hallway + smoke_pulse + 0.2f * odor_pulse, 2.0f, 250.0f);
    frame->corridor_pm10_ugm3 = clampf(frame->corridor_pm25_ugm3 * 1.65f + 3.5f, 4.0f, 420.0f);

    frame->pressure_pa = -0.7f + 1.8f * sinf((float)tick * 0.13f) + pressure_pulse + 0.55f * winter_draft;
    if (mode == TV_MODE_SHELTER) {
        frame->pressure_pa += 0.6f;
    }
    if (!frame->door_closed) {
        frame->pressure_pa *= 0.35f;
    }

    frame->threshold_temp_c = frame->indoor_temp_c - 0.8f - 0.55f * winter_draft + 0.12f * frame->pressure_pa;
}

float env_pressure_trend(const tv_env_frame_t *frame)
{
    float gradient = frame->pressure_pa;
    if (!frame->latch_aligned) {
        gradient += 1.1f;
    }
    if (!frame->door_closed) {
        gradient *= 0.4f;
    }
    return gradient;
}

float env_contaminant_delta(const tv_env_frame_t *frame)
{
    const float pm_term = 0.65f * (frame->corridor_pm25_ugm3 / 35.0f);
    const float voc_term = 0.45f * ((frame->corridor_voc_index - frame->indoor_voc_index) / 80.0f);
    return clampf(pm_term + voc_term, -1.0f, 4.0f);
}

float env_draft_delta(const tv_env_frame_t *frame)
{
    const float temp_gap = frame->indoor_temp_c - frame->threshold_temp_c;
    const float pressure = fabsf(frame->pressure_pa) * 0.25f;
    return clampf(temp_gap + pressure, 0.0f, 12.0f);
}
