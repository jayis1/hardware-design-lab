/*
 * CrisperCue gas sensing driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "gas.h"

static float profile_ethylene_base(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return 0.04f;
    case BIN_PROFILE_BERRIES: return 0.07f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 0.24f;
    default: return 0.05f;
    }
}

static float profile_humidity_target(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return 94.0f;
    case BIN_PROFILE_BERRIES: return 91.0f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 87.0f;
    default: return 90.0f;
    }
}

void gas_init(gas_state_t *state, bin_profile_t profile)
{
    state->co2_ppm = 860.0f;
    state->ethylene_ppm = profile_ethylene_base(profile);
    state->voc_index = 28.0f;
    state->oxygen_percent = 20.4f;
    state->humidity_rh = profile_humidity_target(profile);
    state->purge_efficiency = 0.82f;
}

void gas_sample(gas_state_t *state, bin_profile_t profile, uint32_t cycle, const thermal_state_t *thermal)
{
    const float ethylene_base = profile_ethylene_base(profile);
    const float humidity_target = profile_humidity_target(profile);
    const float respiration = 0.4f + 0.03f * (float)cycle + 0.08f * thermal->drawer_open_minutes;
    const float thermal_push = fmaxf(0.0f, thermal->produce_temp_c - 4.5f);

    state->co2_ppm = 780.0f + 78.0f * respiration + 24.0f * (float)(cycle % 3u) + 35.0f * thermal_push;
    state->ethylene_ppm = ethylene_base + 0.011f * (float)cycle + 0.020f * thermal_push;
    state->voc_index = 24.0f + 2.2f * respiration + 4.0f * thermal_push;
    state->oxygen_percent = 20.8f - 0.05f * (float)cycle - 0.08f * thermal_push;
    state->humidity_rh = humidity_target - 0.22f * thermal->drawer_open_minutes + 0.18f * sinf((float)cycle * 0.35f);
    state->purge_efficiency = 0.89f - 0.012f * (float)(cycle / 6u) - 0.015f * thermal_push;

    if (profile == BIN_PROFILE_CLIMACTERIC_FRUIT && cycle > 18u) {
        state->ethylene_ppm += 0.20f;
        state->co2_ppm += 90.0f;
        state->voc_index += 8.0f;
    }
    if (cycle > 28u) {
        state->oxygen_percent -= 0.45f;
        state->humidity_rh -= 1.6f;
        state->purge_efficiency -= 0.08f;
    }
}

const char *gas_air_quality_label(const gas_state_t *state)
{
    if (state->ethylene_ppm > 0.70f || state->co2_ppm > 1500.0f) {
        return "ripening-spike";
    }
    if (state->humidity_rh < 84.0f || state->oxygen_percent < 19.0f) {
        return "stale-bin";
    }
    return "stable-bin";
}
