/*
 * DrainVeil pressure driver simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "pressure.h"

static float pulse(uint32_t minute, float speed, float phase)
{
    return cosf(((float)minute * speed) + phase);
}

void pressure_init(pressure_state_t *state, install_profile_t profile)
{
    state->line_pressure_kpa = dv_profile_bias(profile, 1.8f, 0.6f, 2.6f);
    state->pulse_variance = 0.08f;
    state->water_hammer_score = 0.06f;
    state->trap_oscillation = 0.10f;
    state->vibration_rms = 0.12f;
    state->blockage_gradient = 0.15f;
    state->branch_asymmetry = 0.14f;
}

void pressure_sample(pressure_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow)
{
    float p1 = 0.5f + 0.5f * pulse(minute, 0.20f, 0.2f);
    float p2 = 0.5f + 0.5f * pulse(minute, 0.47f, 1.0f);
    float loading = flow->fill_height_percent / 100.0f;
    float turbulence = flow->turbulence_index;

    state->line_pressure_kpa = dv_profile_bias(profile,
                                               1.1f + 3.8f * loading + 0.9f * turbulence,
                                               0.2f + 1.7f * loading + 0.5f * turbulence,
                                               1.8f + 5.2f * loading + 1.0f * turbulence);
    state->pulse_variance = dv_clampf(0.05f + 0.22f * p1 + 0.18f * turbulence, 0.0f, 1.0f);
    state->water_hammer_score = dv_clampf(0.04f + 0.18f * p2 + 0.22f * flow->bubble_factor, 0.0f, 1.0f);
    state->trap_oscillation = dv_clampf(0.08f + 0.25f * p1 + 0.18f * flow->slug_probability, 0.0f, 1.0f);
    state->vibration_rms = dv_clampf(0.05f + 0.30f * turbulence + 0.22f * p2, 0.0f, 1.0f);
    state->blockage_gradient = dv_clampf(0.12f + 0.48f * loading + 0.20f * flow->reflection_strength, 0.0f, 1.0f);
    state->branch_asymmetry = dv_clampf(0.10f + 0.26f * fabsf(p1 - p2) + 0.16f * loading, 0.0f, 1.0f);

    if (minute >= 18u) {
        state->line_pressure_kpa += 0.25f * (float)(minute - 17u);
        state->blockage_gradient = dv_clampf(state->blockage_gradient + 0.03f * (float)(minute - 17u), 0.0f, 1.0f);
        state->trap_oscillation = dv_clampf(state->trap_oscillation + 0.02f * (float)(minute - 17u), 0.0f, 1.0f);
    }
}

const char *pressure_pattern_label(const pressure_state_t *state)
{
    if (state->blockage_gradient > 0.78f) return "backpressure-rise";
    if (state->water_hammer_score > 0.52f) return "hammering";
    if (state->trap_oscillation > 0.42f) return "trap-breathing";
    return "stable";
}
