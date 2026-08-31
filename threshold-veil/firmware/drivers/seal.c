/*
 * Threshold Veil seal driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "seal.h"

#include <math.h>

#include "inference.h"

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

void seal_init(tv_seal_frame_t *frame)
{
    frame->louver = TV_LOUVER_SAMPLE;
    frame->blower_pwm = 0.0f;
    frame->gasket_pressure_kpa = 2.2f;
    frame->target_pressure_kpa = 2.2f;
    frame->seal_health_pct = 98.0f;
    frame->relief_valve_open = false;
}

void seal_apply(tv_seal_frame_t *frame, const tv_inference_t *inf, const tv_env_frame_t *env, tv_mode_t mode)
{
    float target = 2.2f;
    float blower = 0.12f;
    tv_louver_t louver = TV_LOUVER_SAMPLE;
    bool relief = false;

    if (!env->door_closed) {
        target = 0.6f;
        blower = 0.0f;
        louver = TV_LOUVER_EQUALIZE;
        relief = true;
    } else if (mode == TV_MODE_OPEN_FLOW) {
        target = 1.0f;
        blower = 0.05f;
        louver = TV_LOUVER_EQUALIZE;
    } else if (inf->state == TV_STATE_SMOKE_PUSH || inf->state == TV_STATE_SHELTER) {
        target = 8.9f;
        blower = 0.96f;
        louver = TV_LOUVER_SEAL;
    } else if (inf->state == TV_STATE_ODOR_PUSH) {
        target = 6.1f;
        blower = 0.58f;
        louver = TV_LOUVER_SEAL;
    } else if (inf->state == TV_STATE_QUIET_HOURS) {
        target = 5.2f;
        blower = 0.42f;
        louver = TV_LOUVER_SEAL;
    } else if (inf->state == TV_STATE_PRESSURE_SURGE) {
        target = 6.8f;
        blower = 0.66f;
        louver = TV_LOUVER_SEAL;
    }

    if (!env->latch_aligned) {
        target += 0.8f;
        blower += 0.07f;
    }

    frame->target_pressure_kpa = clampf(target, 0.4f, 9.5f);
    frame->blower_pwm = clampf(blower, 0.0f, 1.0f);
    frame->louver = louver;
    frame->relief_valve_open = relief;

    const float response = frame->blower_pwm * 2.4f + (frame->louver == TV_LOUVER_SEAL ? 1.1f : -0.2f);
    frame->gasket_pressure_kpa = clampf(frame->gasket_pressure_kpa + 0.35f * (frame->target_pressure_kpa - frame->gasket_pressure_kpa) + 0.08f * response,
                                        0.0f,
                                        10.0f);

    const float mismatch = fabsf(frame->target_pressure_kpa - frame->gasket_pressure_kpa);
    frame->seal_health_pct = clampf(frame->seal_health_pct - 0.08f * mismatch + 0.02f * (env->door_closed ? 1.0f : 0.0f),
                                    74.0f,
                                    99.0f);
}

float seal_effectiveness(const tv_seal_frame_t *frame, const tv_env_frame_t *env)
{
    float effectiveness = (frame->gasket_pressure_kpa / 9.5f) * 2.1f;
    if (frame->louver == TV_LOUVER_SEAL) {
        effectiveness += 0.7f;
    }
    if (!env->latch_aligned) {
        effectiveness -= 0.35f;
    }
    if (!env->door_closed) {
        effectiveness *= 0.15f;
    }
    return clampf(effectiveness, 0.0f, 3.0f);
}
