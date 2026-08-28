/*
 * VentLattice power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "power.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void power_init(power_state_t *state, room_profile_t profile)
{
    (void)profile;
    state->battery_mv = 4088.0f;
    state->battery_percent = 96.0f;
    state->current_ma = 0.9f;
    state->runtime_hours_est = 1480.0f;
    state->charging = 0u;
}

void power_update(power_state_t *state, const power_state_t *previous, const vent_snapshot_t *snapshot)
{
    float activity = 0.8f + snapshot->airflow.hvac_call_active * 0.9f + snapshot->occupancy.occupied_now * 0.5f + snapshot->pressure.turbulence_index * 0.6f;
    state->current_ma = 0.7f + activity;
    state->battery_mv = clampf(previous->battery_mv - state->current_ma * 0.42f, 3600.0f, 4200.0f);
    state->battery_percent = clampf((state->battery_mv - 3600.0f) / 6.0f, 0.0f, 100.0f);
    state->runtime_hours_est = clampf(state->battery_percent * 14.5f / (state->current_ma * 0.52f), 0.0f, 2200.0f);
    state->charging = 0u;
}

float power_status_register(const power_state_t *state)
{
    return state->charging ? 2.0f : (state->battery_percent < 15.0f ? 1.0f : 0.0f);
}
