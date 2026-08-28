/*
 * VentLattice occupancy driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "occupancy.h"

static float clampf(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

void occupancy_init(occupancy_state_t *state, room_profile_t profile)
{
    (void)profile;
    state->presence_confidence = 0.08f;
    state->dwell_hours = 0.0f;
    state->occupied_alignment = 0.5f;
    state->occupied_now = 0u;
}

void occupancy_sample(occupancy_state_t *state, room_profile_t profile, uint32_t hour_index, const environment_state_t *environment)
{
    float scheduled_presence = 0.0f;
    switch (profile) {
    case ROOM_PROFILE_HOME_OFFICE:
        scheduled_presence = (hour_index >= 8u && hour_index <= 17u) ? 0.88f : ((hour_index >= 20u && hour_index <= 22u) ? 0.35f : 0.08f);
        break;
    case ROOM_PROFILE_NURSERY:
        scheduled_presence = (hour_index <= 6u || hour_index >= 19u) ? 0.76f : 0.20f;
        break;
    case ROOM_PROFILE_CLASSROOM:
        scheduled_presence = (hour_index >= 8u && hour_index <= 15u) ? 0.94f : 0.05f;
        break;
    default:
        scheduled_presence = 0.10f;
        break;
    }

    state->presence_confidence = clampf(scheduled_presence + (environment->voc_index - 100.0f) * 0.002f + environment->light_lux * 0.0002f, 0.0f, 1.0f);
    state->occupied_now = (uint8_t)(state->presence_confidence > 0.45f);
    if (state->occupied_now) {
        state->dwell_hours = clampf(state->dwell_hours + 1.0f, 0.0f, 18.0f);
    } else {
        state->dwell_hours = clampf(state->dwell_hours - 0.6f, 0.0f, 18.0f);
    }
    state->occupied_alignment = clampf(0.18f + state->presence_confidence * 0.82f, 0.0f, 1.0f);
}
