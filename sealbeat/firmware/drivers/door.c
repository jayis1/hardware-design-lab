/*
 * SealBeat door kinematics driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "door.h"

static float open_duration_pattern(uint32_t minute)
{
    if (minute % 12u == 0u) return 34.0f;
    if (minute % 7u == 0u) return 18.0f;
    if (minute % 5u == 0u) return 12.0f;
    return 4.0f + (float)(minute % 4u) * 2.5f;
}

void door_init(door_state_t *state, appliance_profile_t profile)
{
    state->door_angle_deg = 0.0f;
    state->dwell_open_seconds = 3.0f;
    state->bounce_count = 0.02f;
    state->close_velocity = sb_profile_bias(profile, 0.32f, 0.36f, 0.28f);
    state->hinge_skew = sb_profile_bias(profile, 0.08f, 0.10f, 0.05f);
    state->tilt_drift_deg = 0.0f;
    state->cycle_count = 0u;
    state->night_cycles = 0u;
}

void door_sample(door_state_t *state, appliance_profile_t profile, uint32_t minute)
{
    const float duration = open_duration_pattern(minute) + sb_profile_bias(profile, 0.0f, 2.0f, -1.0f);
    const float high_traffic = (minute % 9u == 4u) ? 1.0f : 0.0f;
    state->cycle_count += 1u;
    if ((minute % 16u) < 3u) {
        state->night_cycles += 1u;
    }
    state->door_angle_deg = sb_clampf(12.0f + duration * 1.8f + high_traffic * 24.0f, 0.0f, 95.0f);
    state->dwell_open_seconds = duration;
    state->bounce_count = sb_clampf(0.04f + (float)(minute % 6u) * 0.05f + high_traffic * 0.12f + (minute > 28u ? 0.08f : 0.0f), 0.0f, 1.0f);
    state->close_velocity = sb_clampf(0.26f + (float)(minute % 5u) * 0.07f + high_traffic * 0.14f, 0.0f, 1.0f);
    state->hinge_skew = sb_clampf(state->hinge_skew + 0.0025f + (minute > 26u ? 0.0030f : 0.0f), 0.0f, 1.0f);
    state->tilt_drift_deg = sb_clampf(state->tilt_drift_deg + (minute % 10u == 0u ? 0.04f : 0.01f), 0.0f, 4.0f);
}

const char *door_pattern_label(const door_state_t *state)
{
    if (state->bounce_count > 0.42f) return "bounce";
    if (state->dwell_open_seconds > 24.0f) return "long-open";
    if (state->hinge_skew > 0.36f) return "hinge-drift";
    return "normal-use";
}
