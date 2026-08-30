/*
 * CrisperCue thermal modeling driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "thermal.h"

static float profile_temp_bias(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return -0.6f;
    case BIN_PROFILE_BERRIES: return -0.2f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 0.4f;
    default: return 0.0f;
    }
}

void thermal_init(thermal_state_t *state, bin_profile_t profile)
{
    state->air_temp_c = 3.9f + profile_temp_bias(profile);
    state->produce_temp_c = state->air_temp_c + 0.8f;
    state->dew_margin_c = 2.5f;
    state->compressor_cycles = 1.2f;
    state->drawer_open_minutes = 3.0f;
}

void thermal_sample(thermal_state_t *state, bin_profile_t profile, uint32_t cycle)
{
    const float rhythm = sinf((float)cycle * 0.41f);
    const float disturbance = cosf((float)cycle * 0.23f);
    const float bias = profile_temp_bias(profile);

    state->compressor_cycles = 1.0f + 0.35f * fabsf(rhythm) + 0.08f * (float)(cycle % 4u);
    state->drawer_open_minutes = 1.5f + 0.9f * (float)(cycle % 5u) + 1.2f * fmaxf(0.0f, disturbance);
    state->air_temp_c = 4.1f + bias + 0.7f * rhythm + 0.05f * state->drawer_open_minutes;
    state->produce_temp_c = state->air_temp_c + 0.6f + 0.2f * fabsf(disturbance);
    state->dew_margin_c = 2.8f - 0.32f * state->drawer_open_minutes + 0.25f * rhythm;

    if (cycle > 24u) {
        state->air_temp_c += 0.4f;
        state->produce_temp_c += 0.5f;
        state->dew_margin_c -= 0.3f;
    }
    if (cycle > 30u) {
        state->drawer_open_minutes += 1.5f;
        state->dew_margin_c -= 0.4f;
    }
}

const char *thermal_door_label(const thermal_state_t *state)
{
    if (state->drawer_open_minutes > 6.5f) {
        return "busy-kitchen";
    }
    if (state->drawer_open_minutes > 4.0f) {
        return "normal-use";
    }
    return "sealed";
}
