/*
 * CrisperCue optical modeling driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "optical.h"

static float profile_color_start(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return 0.82f;
    case BIN_PROFILE_BERRIES: return 0.76f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 0.58f;
    default: return 0.70f;
    }
}

void optical_init(optical_state_t *state, bin_profile_t profile)
{
    state->color_index = profile_color_start(profile);
    state->chlorophyll_index = profile == BIN_PROFILE_LEAFY_GREENS ? 0.86f : 0.52f;
    state->bruise_probability = 0.04f;
    state->mold_signature = 0.03f;
    state->surface_gloss = 0.72f;
}

void optical_sample(optical_state_t *state, bin_profile_t profile, uint32_t cycle, const gas_state_t *gas, const mass_state_t *mass)
{
    const float decay = 0.012f * (float)cycle + gas->ethylene_ppm * 0.04f;
    const float dryness = fmaxf(0.0f, 90.0f - gas->humidity_rh) * 0.01f;
    const float bruising = (mass->usage_velocity * 0.06f) + 0.01f * (float)(cycle % 4u);

    state->color_index = fmaxf(0.08f, profile_color_start(profile) - decay + 0.03f * sinf((float)cycle * 0.3f));
    state->chlorophyll_index = fmaxf(0.02f, state->chlorophyll_index - 0.009f * (float)cycle - 0.02f * dryness);
    state->bruise_probability = fminf(0.98f, 0.03f + bruising + gas->voc_index / 600.0f);
    state->mold_signature = fminf(0.99f, 0.02f + fmaxf(0.0f, gas->humidity_rh - 90.0f) / 180.0f + gas->co2_ppm / 5000.0f);
    state->surface_gloss = fmaxf(0.05f, 0.74f - dryness * 0.4f - 0.01f * (float)cycle);

    if (profile == BIN_PROFILE_BERRIES && cycle > 20u) {
        state->mold_signature += 0.12f;
    }
    if (profile == BIN_PROFILE_CLIMACTERIC_FRUIT && cycle > 18u) {
        state->color_index += 0.08f;
        state->surface_gloss -= 0.06f;
    }
}

const char *optical_stage_hint(const optical_state_t *state)
{
    if (state->mold_signature > 0.70f) {
        return "inspect-surface";
    }
    if (state->color_index < 0.24f || state->surface_gloss < 0.20f) {
        return "past-peak";
    }
    if (state->bruise_probability > 0.48f) {
        return "handle-gently";
    }
    return "camera-clear";
}
