/*
 * DrainVeil thermal driver simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "thermal.h"

static float thermal_wave(uint32_t minute, float speed, float phase)
{
    return 0.5f + 0.5f * cosf(((float)minute * speed) + phase);
}

void thermal_init(thermal_state_t *state, install_profile_t profile)
{
    state->pipe_temp_c = dv_profile_bias(profile, 22.0f, 14.0f, 28.0f);
    state->ambient_temp_c = dv_profile_bias(profile, 24.0f, 16.0f, 30.0f);
    state->freeze_margin_c = dv_profile_bias(profile, 18.0f, 7.0f, 23.0f);
    state->thermal_recovery_s = 18.0f;
    state->heat_leak_score = 0.18f;
    state->cold_slug_index = 0.08f;
}

void thermal_sample(thermal_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow, const chemistry_state_t *chemistry)
{
    float ambient = thermal_wave(minute, 0.14f, 0.5f);
    float chill = thermal_wave(minute, 0.22f, 1.1f);
    float load = flow->fill_height_percent / 100.0f;

    state->ambient_temp_c = dv_profile_bias(profile, 22.0f + 6.0f * ambient,
                                            8.0f + 10.0f * ambient,
                                            26.0f + 7.0f * ambient);
    state->pipe_temp_c = state->ambient_temp_c - dv_profile_bias(profile, 2.5f, 5.0f + 5.0f * chill, 1.5f) + 1.4f * load;
    state->freeze_margin_c = dv_clampf(state->pipe_temp_c - 0.0f, -12.0f, 40.0f);
    state->thermal_recovery_s = dv_clampf(8.0f + 24.0f * load + 10.0f * chemistry->condensate_risk, 4.0f, 80.0f);
    state->heat_leak_score = dv_clampf(0.08f + 0.35f * ambient + 0.20f * chemistry->humidity_percent / 100.0f, 0.0f, 1.0f);
    state->cold_slug_index = dv_clampf(0.05f + 0.22f * chill + 0.18f * flow->slug_probability, 0.0f, 1.0f);

    if (profile == INSTALL_PROFILE_FLOOR_DRAIN && minute > 18u) {
        state->pipe_temp_c -= 0.8f * (float)(minute - 18u);
        state->freeze_margin_c = state->pipe_temp_c;
        state->cold_slug_index = dv_clampf(state->cold_slug_index + 0.04f * (float)(minute - 18u), 0.0f, 1.0f);
    }
}

const char *thermal_status_label(const thermal_state_t *state)
{
    if (state->freeze_margin_c < 1.5f) return "freeze-risk";
    if (state->heat_leak_score > 0.62f) return "warm-envelope";
    if (state->cold_slug_index > 0.48f) return "cold-shock";
    return "steady-temp";
}
