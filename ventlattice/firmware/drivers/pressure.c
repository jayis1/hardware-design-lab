/*
 * VentLattice pressure driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "pressure.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void pressure_init(pressure_state_t *state, room_profile_t profile)
{
    (void)profile;
    state->ripple_pa = 1.2f;
    state->blower_signature = 0.28f;
    state->filter_load_index = 0.18f;
    state->branch_restriction = 0.10f;
    state->turbulence_index = 0.16f;
}

void pressure_sample(pressure_state_t *state, room_profile_t profile, uint32_t hour_index, const airflow_state_t *airflow)
{
    float scale = (profile == ROOM_PROFILE_CLASSROOM) ? 1.15f : 1.0f;
    float active_bonus = airflow->hvac_call_active ? 1.0f : 0.22f;
    float blockage = airflow->blockage_index;
    float cycle = 0.12f * sinf((float)hour_index * 0.55f);

    state->ripple_pa = clampf((0.8f + airflow->nozzle_delta_pa * 0.16f + blockage * 3.4f + cycle) * scale * active_bonus, 0.2f, 19.0f);
    state->blower_signature = clampf(0.24f + airflow->velocity_mps * 0.12f + blockage * 0.20f, 0.0f, 1.0f);
    state->filter_load_index = clampf(0.22f + (hour_index > 10u ? 0.01f * (float)(hour_index - 10u) : 0.0f) + blockage * 0.32f, 0.0f, 1.0f);
    state->branch_restriction = clampf(blockage * 0.86f + (1.0f - airflow->delivery_stability) * 0.28f, 0.0f, 1.0f);
    state->turbulence_index = clampf(0.12f + blockage * 0.52f + airflow->velocity_mps * 0.04f + fabsf(cycle), 0.0f, 1.0f);
}
