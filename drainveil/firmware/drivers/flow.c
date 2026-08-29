/*
 * DrainVeil flow driver simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "flow.h"

static float wave(uint32_t minute, float speed, float phase)
{
    return sinf(((float)minute * speed) + phase);
}

static float profile_offset(install_profile_t profile)
{
    switch (profile) {
    case INSTALL_PROFILE_KITCHEN_SINK: return 0.10f;
    case INSTALL_PROFILE_FLOOR_DRAIN: return -0.05f;
    case INSTALL_PROFILE_GREASE_INTERCEPTOR: return 0.22f;
    default: return 0.0f;
    }
}

void flow_init(flow_state_t *state, install_profile_t profile)
{
    state->ultrasonic_velocity = 1.55f + profile_offset(profile);
    state->reflection_strength = 0.30f;
    state->turbulence_index = 0.18f;
    state->fill_height_percent = dv_profile_bias(profile, 15.0f, 8.0f, 28.0f);
    state->slug_probability = dv_profile_bias(profile, 0.10f, 0.06f, 0.18f);
    state->flow_lpm = dv_profile_bias(profile, 5.0f, 0.4f, 8.0f);
    state->drain_time_s = dv_profile_bias(profile, 14.0f, 3.0f, 18.0f);
    state->bubble_factor = 0.12f;
}

void flow_sample(flow_state_t *state, install_profile_t profile, uint32_t minute)
{
    float profile_shift = profile_offset(profile);
    float usage_burst = 0.5f + 0.5f * wave(minute, 0.45f, profile_shift);
    float drain_drag = 0.5f + 0.5f * wave(minute, 0.18f, 0.7f);
    float grease_event = 0.5f + 0.5f * wave(minute, 0.08f, 1.4f);

    state->flow_lpm = dv_profile_bias(profile,
                                      4.0f + 8.0f * usage_burst,
                                      0.2f + 1.0f * usage_burst,
                                      6.0f + 10.0f * usage_burst);
    state->fill_height_percent = dv_clampf(dv_profile_bias(profile,
                                                           18.0f + 35.0f * drain_drag,
                                                           10.0f + 22.0f * drain_drag,
                                                           30.0f + 42.0f * drain_drag)
                                           + 12.0f * grease_event * (profile_shift + 0.3f), 2.0f, 95.0f);
    state->turbulence_index = dv_clampf(0.18f + 0.52f * usage_burst + 0.15f * grease_event, 0.02f, 0.98f);
    state->reflection_strength = dv_clampf(0.22f + 0.58f * drain_drag + 0.10f * grease_event, 0.05f, 0.99f);
    state->slug_probability = dv_clampf(0.08f + 0.45f * drain_drag + 0.12f * grease_event, 0.01f, 0.99f);
    state->ultrasonic_velocity = 1.48f + 0.32f * usage_burst - 0.22f * grease_event + 0.11f * profile_shift;
    state->drain_time_s = dv_clampf(4.0f + 22.0f * state->slug_probability + 0.35f * state->fill_height_percent, 2.0f, 58.0f);
    state->bubble_factor = dv_clampf(0.09f + 0.28f * usage_burst + 0.12f * drain_drag, 0.01f, 0.95f);

    if (minute > 16u) {
        state->fill_height_percent = dv_clampf(state->fill_height_percent + 0.6f * (float)(minute - 16u), 2.0f, 95.0f);
        state->drain_time_s = dv_clampf(state->drain_time_s + 1.1f * (float)(minute - 16u), 2.0f, 58.0f);
        state->slug_probability = dv_clampf(state->slug_probability + 0.02f * (float)(minute - 16u), 0.0f, 1.0f);
    }
}

const char *flow_pattern_label(const flow_state_t *state)
{
    if (state->fill_height_percent > 70.0f || state->drain_time_s > 34.0f) return "slow-clear";
    if (state->turbulence_index > 0.72f && state->bubble_factor > 0.32f) return "gurgling";
    if (state->flow_lpm < 0.8f && state->reflection_strength > 0.60f) return "standing-column";
    return "nominal";
}
