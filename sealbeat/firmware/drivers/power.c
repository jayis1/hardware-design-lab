/*
 * SealBeat power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "power.h"

void power_init(power_state_t *state, appliance_profile_t profile)
{
    state->battery_mv = 4050.0f;
    state->battery_percent = 96.0f;
    state->charge_current_ma = 0.0f;
    state->average_current_ma = sb_profile_bias(profile, 0.48f, 0.52f, 0.58f);
    state->estimated_days_left = 240.0f;
    state->charging = 0u;
    state->low_power_mode = 0u;
}

void power_update(power_state_t *state, const power_state_t *previous, const appliance_snapshot_t *snapshot)
{
    const float activity = snapshot->door.dwell_open_seconds * 0.010f + snapshot->acoustic.compressor_burden * 0.15f + (1.0f - snapshot->seal.closure_confidence) * 0.20f;
    state->average_current_ma = sb_clampf(previous->average_current_ma * 0.7f + activity * 0.3f + 0.35f, 0.25f, 3.50f);
    state->battery_percent = sb_clampf(previous->battery_percent - state->average_current_ma * 0.12f, 0.0f, 100.0f);
    state->battery_mv = 3200.0f + state->battery_percent * 8.4f;
    state->estimated_days_left = sb_clampf((state->battery_percent / (state->average_current_ma + 0.05f)) * 1.8f, 0.0f, 365.0f);
    state->charge_current_ma = 0.0f;
    state->charging = 0u;
    state->low_power_mode = state->battery_percent < 18.0f ? 1u : 0u;
}

float power_status_register(const power_state_t *state)
{
    return (state->charging ? 0x01u : 0x00u) | (state->low_power_mode ? 0x02u : 0x00u);
}
