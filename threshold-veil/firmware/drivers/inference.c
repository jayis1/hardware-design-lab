/*
 * Threshold Veil inference driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "inference.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "acoustic.h"
#include "environment.h"
#include "seal.h"

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

static void set_recommendation(tv_inference_t *inf, const char *text)
{
    snprintf(inf->recommendation, sizeof(inf->recommendation), "%s", text);
}

void inference_init(tv_inference_t *inf)
{
    memset(inf, 0, sizeof(*inf));
    inf->state = TV_STATE_CALM;
    set_recommendation(inf, "Boundary stable. Auto-watch mode active.");
}

void inference_evaluate(tv_inference_t *inf,
                        const tv_env_frame_t *env,
                        const tv_acoustic_frame_t *ac,
                        const tv_seal_frame_t *seal,
                        tv_mode_t mode)
{
    const float pressure = env_pressure_trend(env);
    const float contam = env_contaminant_delta(env);
    const float draft = env_draft_delta(env);
    const float acoustic = acoustic_leak_index(ac);
    const float seal_gain = seal_effectiveness(seal, env);

    inf->smoke_score = clampf(contam * 1.35f + (env->corridor_pm25_ugm3 / 45.0f) + 0.25f * pressure - 0.30f * seal_gain,
                              0.0f,
                              5.0f);
    inf->odor_score = clampf(((env->corridor_voc_index - env->indoor_voc_index) / 42.0f) + 0.20f * pressure - 0.20f * seal_gain,
                             0.0f,
                             5.0f);
    inf->draft_score = clampf((draft / 3.4f) + 0.18f * fabsf(pressure) - 0.22f * seal_gain,
                              0.0f,
                              5.0f);
    inf->acoustic_score = clampf(acoustic + (env->quiet_hours ? 0.35f : 0.0f) - 0.25f * seal_gain,
                                 0.0f,
                                 5.0f);

    inf->ingress_score = clampf(0.42f * inf->smoke_score +
                                0.28f * inf->odor_score +
                                0.18f * inf->draft_score +
                                0.12f * inf->acoustic_score,
                                0.0f,
                                5.0f);

    inf->confidence = clampf(0.56f + 0.06f * fabsf(pressure) + 0.04f * contam + 0.05f * ac->transient_score,
                             0.45f,
                             0.99f);
    inf->recommend_push_alert = false;

    if (!env->door_closed) {
        inf->state = TV_STATE_DOOR_OPEN;
        set_recommendation(inf, "Door open. Equalizing and reducing seal pressure until latch closes.");
        return;
    }

    if (mode == TV_MODE_SHELTER || inf->smoke_score > 2.85f) {
        inf->state = TV_STATE_SHELTER;
        inf->recommend_push_alert = true;
        set_recommendation(inf, "Shelter mode: keep door closed, maintain seal, and reduce corridor exposure.");
        return;
    }

    if (inf->smoke_score > 1.95f && pressure > 0.4f) {
        inf->state = TV_STATE_SMOKE_PUSH;
        inf->recommend_push_alert = true;
        set_recommendation(inf, "Hallway smoke or aerosol ingress detected. Inflate full perimeter and close sample louver.");
        return;
    }

    if (fabsf(pressure) > 5.4f && inf->draft_score > 0.95f) {
        inf->state = TV_STATE_PRESSURE_SURGE;
        set_recommendation(inf, "Pressure surge detected. Preload latch-side and threshold seal chambers.");
        return;
    }

    if ((inf->odor_score > 1.15f && pressure > 0.2f) || (env->corridor_voc_index - env->indoor_voc_index) > 38.0f) {
        inf->state = TV_STATE_ODOR_PUSH;
        set_recommendation(inf, "Odor ingress without high smoke risk. Seal comfort path and monitor particulate rise.");
        return;
    }

    if ((mode == TV_MODE_QUIET || env->quiet_hours) && inf->acoustic_score > 0.90f) {
        inf->state = TV_STATE_QUIET_HOURS;
        set_recommendation(inf, "Quiet-hours optimization active. Bias seal toward speech and cart-noise attenuation.");
        return;
    }

    if (seal->seal_health_pct < 80.0f || !env->latch_aligned) {
        inf->state = TV_STATE_SERVICE;
        set_recommendation(inf, "Seal wear or latch misalignment detected. Recalibrate or inspect gasket strip.");
        return;
    }

    inf->state = TV_STATE_CALM;
    set_recommendation(inf, "Boundary stable. Watch mode tracking indoor and corridor gradients.");
}

const char *inference_state_name(tv_state_t state)
{
    switch (state) {
        case TV_STATE_CALM:
            return "CALM";
        case TV_STATE_ODOR_PUSH:
            return "ODOR_PUSH";
        case TV_STATE_SMOKE_PUSH:
            return "SMOKE_PUSH";
        case TV_STATE_QUIET_HOURS:
            return "QUIET_HOURS";
        case TV_STATE_PRESSURE_SURGE:
            return "PRESSURE_SURGE";
        case TV_STATE_SHELTER:
            return "SHELTER";
        case TV_STATE_DOOR_OPEN:
            return "DOOR_OPEN";
        case TV_STATE_SERVICE:
            return "SERVICE";
        default:
            return "UNKNOWN";
    }
}
