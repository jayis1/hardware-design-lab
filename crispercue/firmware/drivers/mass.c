/*
 * CrisperCue mass driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "mass.h"

static float profile_start_mass(bin_profile_t profile)
{
    switch (profile) {
    case BIN_PROFILE_LEAFY_GREENS: return 510.0f;
    case BIN_PROFILE_BERRIES: return 340.0f;
    case BIN_PROFILE_CLIMACTERIC_FRUIT: return 890.0f;
    default: return 500.0f;
    }
}

void mass_init(mass_state_t *state, bin_profile_t profile)
{
    state->tray_mass_g = profile_start_mass(profile);
    state->daily_loss_g = 4.2f;
    state->moisture_loss_percent = 0.8f;
    state->usage_velocity = 0.22f;
    state->refill_detected = 0u;
}

void mass_sample(mass_state_t *state, bin_profile_t profile, uint32_t cycle, const gas_state_t *gas)
{
    const float start_mass = profile_start_mass(profile);
    const float usage_pulse = 12.0f * (float)((cycle % 7u) == 2u) + 20.0f * (float)((cycle % 9u) == 0u);
    const float respiration_loss = 2.8f + gas->ethylene_ppm * 3.4f + (100.0f - gas->humidity_rh) * 0.08f;
    const float retained = fmaxf(0.0f, state->tray_mass_g - respiration_loss - usage_pulse);

    state->daily_loss_g = respiration_loss;
    state->tray_mass_g = retained;
    state->moisture_loss_percent = fminf(25.0f, ((start_mass - retained) / start_mass) * 100.0f);
    state->usage_velocity = fminf(1.0f, usage_pulse / 22.0f + respiration_loss / 20.0f);
    state->refill_detected = 0u;

    if (cycle == 16u) {
        state->tray_mass_g += 130.0f;
        state->refill_detected = 1u;
    }
    if (cycle == 24u && profile == BIN_PROFILE_BERRIES) {
        state->tray_mass_g += 90.0f;
        state->refill_detected = 1u;
    }
}

const char *mass_usage_label(const mass_state_t *state)
{
    if (state->refill_detected) {
        return "restocked";
    }
    if (state->usage_velocity > 0.8f) {
        return "actively-used";
    }
    if (state->tray_mass_g < 120.0f) {
        return "nearly-empty";
    }
    return "steady-draw";
}
