/*
 * VentLattice airflow driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "airflow.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float profile_base_cfm(room_profile_t profile)
{
    switch (profile) {
    case ROOM_PROFILE_HOME_OFFICE: return 92.0f;
    case ROOM_PROFILE_NURSERY: return 70.0f;
    case ROOM_PROFILE_CLASSROOM: return 165.0f;
    default: return 85.0f;
    }
}

void airflow_init(airflow_state_t *state, room_profile_t profile)
{
    state->airflow_cfm = profile_base_cfm(profile) * 0.15f;
    state->velocity_mps = 0.6f;
    state->blockage_index = 0.08f;
    state->delivery_stability = 0.82f;
    state->vent_open_percent = 0.88f;
    state->nozzle_delta_pa = 4.0f;
    state->hvac_call_active = 0u;
}

void airflow_sample(airflow_state_t *state, room_profile_t profile, uint32_t hour_index)
{
    const float base = profile_base_cfm(profile);
    const int active = ((hour_index >= 6u && hour_index <= 8u) ||
                        (hour_index >= 12u && hour_index <= 17u) ||
                        (hour_index >= 20u && hour_index <= 22u));
    float demand_factor = active ? 1.0f : 0.18f;
    float circadian = 0.08f * sinf((float)hour_index * 0.45f);
    float solar_penalty = (hour_index >= 14u && hour_index <= 17u) ? 0.10f : 0.0f;
    float blockage_growth = (hour_index >= 15u) ? 0.02f * (float)(hour_index - 14u) : 0.0f;
    float open_factor = 0.90f - solar_penalty;

    state->hvac_call_active = (uint8_t)active;
    state->blockage_index = clampf(0.08f + blockage_growth + (active ? 0.03f : 0.0f), 0.0f, 0.92f);
    state->vent_open_percent = clampf(open_factor - state->blockage_index * 0.18f, 0.32f, 0.94f);
    state->airflow_cfm = base * demand_factor * state->vent_open_percent * (1.0f - 0.58f * state->blockage_index + circadian);
    if (!active) {
        state->airflow_cfm += 4.0f * (1.0f - state->blockage_index);
    }
    state->airflow_cfm = clampf(state->airflow_cfm, 6.0f, base * 1.08f);
    state->velocity_mps = clampf(state->airflow_cfm / 53.0f, 0.2f, 5.8f);
    state->nozzle_delta_pa = clampf(2.5f + state->velocity_mps * 4.1f + state->blockage_index * 11.0f, 1.0f, 42.0f);
    state->delivery_stability = clampf(0.91f - state->blockage_index * 0.42f - (active ? 0.03f : 0.0f) + 0.02f * cosf((float)hour_index), 0.22f, 0.98f);
}

const char *airflow_state_label(const airflow_state_t *state)
{
    if (!state->hvac_call_active && state->airflow_cfm < 14.0f) return "idle";
    if (state->blockage_index > 0.62f) return "restricted";
    if (state->airflow_cfm > 90.0f) return "strong";
    if (state->airflow_cfm > 45.0f) return "balanced";
    return "light";
}
