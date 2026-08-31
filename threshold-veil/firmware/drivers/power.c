/*
 * Threshold Veil power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "power.h"

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

void power_init(tv_power_frame_t *frame)
{
    frame->battery_voltage = 4.09f;
    frame->battery_pct = 94.0f;
    frame->current_ma = 19.0f;
    frame->est_hours_remaining = 280.0f;
    frame->charging = false;
    frame->thermal_derate = false;
}

void power_step(tv_power_frame_t *frame, const tv_seal_frame_t *seal, const tv_env_frame_t *env, tv_mode_t mode)
{
    float current = 16.0f;
    current += 125.0f * seal->blower_pwm;
    current += 4.5f * seal->gasket_pressure_kpa;
    current += env->door_closed ? 8.0f : 2.0f;
    current += env->quiet_hours ? 5.0f : 0.0f;
    current += (mode == TV_MODE_SHELTER) ? 14.0f : 0.0f;

    frame->thermal_derate = (env->corridor_temp_c > 30.0f || current > 170.0f);
    if (frame->thermal_derate) {
        current *= 0.92f;
    }

    frame->current_ma = current;
    frame->battery_pct = clampf(frame->battery_pct - current / 4800.0f, 6.0f, 100.0f);
    frame->battery_voltage = 3.35f + 0.9f * (frame->battery_pct / 100.0f);
    frame->charging = (env->tick == 0);
    frame->est_hours_remaining = clampf((frame->battery_pct / 100.0f) * (2400.0f / (current > 1.0f ? current : 1.0f)), 4.0f, 420.0f);
}
