/*
 * VentLattice environment driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "environment.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static float room_temp_baseline(room_profile_t profile)
{
    switch (profile) {
    case ROOM_PROFILE_HOME_OFFICE: return 23.9f;
    case ROOM_PROFILE_NURSERY: return 23.0f;
    case ROOM_PROFILE_CLASSROOM: return 24.4f;
    default: return 23.5f;
    }
}

void environment_init(environment_state_t *state, room_profile_t profile)
{
    state->supply_temp_c = 21.0f;
    state->room_temp_c = room_temp_baseline(profile);
    state->humidity_rh = 48.0f;
    state->voc_index = 92.0f;
    state->dew_margin_c = 6.5f;
    state->light_lux = 18.0f;
    state->thermal_need = 0.34f;
}

void environment_sample(environment_state_t *state, room_profile_t profile, uint32_t hour_index, const airflow_state_t *airflow)
{
    float base_room = room_temp_baseline(profile);
    float sunlight = (hour_index >= 13u && hour_index <= 17u) ? 1.0f : 0.0f;
    float occupied_rise = (hour_index >= 8u && hour_index <= 17u) ? 0.7f : 0.0f;
    float cooling_effect = airflow->hvac_call_active ? airflow->airflow_cfm * 0.025f : 0.0f;

    state->light_lux = clampf(10.0f + 280.0f * sunlight + 25.0f * sinf((float)hour_index * 0.4f), 2.0f, 800.0f);
    state->supply_temp_c = clampf(18.8f - airflow->velocity_mps * 0.35f + 0.25f * cosf((float)hour_index * 0.7f), 15.0f, 23.0f);
    state->room_temp_c = clampf(base_room + sunlight * 1.9f + occupied_rise - cooling_effect, 20.0f, 29.0f);
    state->humidity_rh = clampf(46.0f + sunlight * 4.0f + (airflow->hvac_call_active ? -3.0f : 1.5f) + airflow->blockage_index * 5.0f, 30.0f, 72.0f);
    state->voc_index = clampf(88.0f + occupied_rise * 24.0f + airflow->blockage_index * 35.0f - airflow->airflow_cfm * 0.18f, 55.0f, 260.0f);
    state->dew_margin_c = clampf((state->room_temp_c - state->supply_temp_c) * 0.42f - (state->humidity_rh - 50.0f) * 0.06f, -0.5f, 11.5f);
    state->thermal_need = clampf(((state->room_temp_c - 22.5f) * 0.13f) + sunlight * 0.16f + occupied_rise * 0.08f, 0.0f, 1.0f);
}
