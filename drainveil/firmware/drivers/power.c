/*
 * DrainVeil power driver simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "power.h"

void power_init(power_state_t *state, install_profile_t profile)
{
    state->battery_mv = dv_profile_bias(profile, 4080.0f, 4010.0f, 4120.0f);
    state->battery_percent = 98.0f;
    state->solar_reclaim_mah = dv_profile_bias(profile, 0.0f, 4.0f, 0.0f);
    state->estimated_days_left = dv_profile_bias(profile, 260.0f, 300.0f, 220.0f);
    state->rail_noise_mv = 8.0f;
    state->charger_online = 0u;
}

void power_update(power_state_t *state, const power_state_t *previous, const drain_snapshot_t *snapshot)
{
    float burden = 0.35f * snapshot->flow.turbulence_index +
                   0.25f * snapshot->pressure.vibration_rms +
                   0.22f * snapshot->chemistry.humidity_percent / 100.0f +
                   0.18f * snapshot->thermal.heat_leak_score;
    float drain = 4.8f + 10.0f * burden;

    state->battery_mv = previous->battery_mv - drain;
    if (state->battery_mv < 3440.0f) state->battery_mv = 3440.0f;
    state->battery_percent = dv_clampf((state->battery_mv - 3440.0f) / (4200.0f - 3440.0f) * 100.0f, 0.0f, 100.0f);
    state->estimated_days_left = dv_clampf(previous->estimated_days_left - 0.8f - 2.8f * burden, 1.0f, 400.0f);
    state->rail_noise_mv = 7.0f + 12.0f * burden + 10.0f * snapshot->pressure.water_hammer_score;
    state->solar_reclaim_mah = previous->solar_reclaim_mah;
    state->charger_online = state->battery_percent < 14.0f ? 1u : 0u;
}

float power_status_register(const power_state_t *state)
{
    float reg = 0.0f;
    if (state->charger_online) reg += 1.0f;
    if (state->battery_percent < 20.0f) reg += 2.0f;
    if (state->rail_noise_mv > 18.0f) reg += 4.0f;
    return reg;
}
