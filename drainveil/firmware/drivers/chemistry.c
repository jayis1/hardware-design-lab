/*
 * DrainVeil chemistry driver simulation
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <math.h>
#include "chemistry.h"

static float seasonal(uint32_t minute, float speed, float phase)
{
    return 0.5f + 0.5f * sinf(((float)minute * speed) + phase);
}

void chemistry_init(chemistry_state_t *state, install_profile_t profile)
{
    state->humidity_percent = dv_profile_bias(profile, 58.0f, 72.0f, 64.0f);
    state->condensate_risk = 0.16f;
    state->h2s_ppm = dv_profile_bias(profile, 0.6f, 1.8f, 2.3f);
    state->voc_index = dv_profile_bias(profile, 95.0f, 82.0f, 126.0f);
    state->biofilm_proxy = dv_profile_bias(profile, 0.18f, 0.24f, 0.34f);
    state->grease_proxy = dv_profile_bias(profile, 0.14f, 0.06f, 0.42f);
    state->corrosion_index = 0.11f;
}

void chemistry_sample(chemistry_state_t *state, install_profile_t profile, uint32_t minute, const flow_state_t *flow, const pressure_state_t *pressure)
{
    float wetness = seasonal(minute, 0.17f, 0.6f);
    float anaerobic = seasonal(minute, 0.11f, 1.3f);
    float load = flow->fill_height_percent / 100.0f;

    state->humidity_percent = dv_clampf(dv_profile_bias(profile, 52.0f, 68.0f, 60.0f) + 22.0f * wetness + 7.0f * load, 32.0f, 98.0f);
    state->condensate_risk = dv_clampf(0.10f + 0.28f * wetness + 0.22f * load, 0.0f, 1.0f);
    state->h2s_ppm = dv_clampf(dv_profile_bias(profile, 0.4f, 1.2f, 1.9f) + 2.8f * anaerobic * load + 1.5f * pressure->trap_oscillation, 0.0f, 25.0f);
    state->voc_index = dv_clampf(dv_profile_bias(profile, 88.0f, 70.0f, 112.0f) + 110.0f * load + 55.0f * anaerobic, 20.0f, 500.0f);
    state->biofilm_proxy = dv_clampf(0.10f + 0.42f * load + 0.18f * anaerobic, 0.0f, 1.0f);
    state->grease_proxy = dv_clampf(dv_profile_bias(profile, 0.12f, 0.04f, 0.38f) + 0.40f * load + 0.12f * flow->reflection_strength, 0.0f, 1.0f);
    state->corrosion_index = dv_clampf(0.08f + 0.26f * (state->h2s_ppm / 8.0f) + 0.18f * state->condensate_risk, 0.0f, 1.0f);

    if (minute > 17u) {
        state->biofilm_proxy = dv_clampf(state->biofilm_proxy + 0.02f * (float)(minute - 17u), 0.0f, 1.0f);
        state->h2s_ppm += 0.25f * (float)(minute - 17u);
        state->voc_index += 6.0f * (float)(minute - 17u);
    }
}

const char *chemistry_status_label(const chemistry_state_t *state)
{
    if (state->h2s_ppm > 4.0f || state->voc_index > 220.0f) return "odor-plume";
    if (state->grease_proxy > 0.68f) return "fat-buildup";
    if (state->condensate_risk > 0.56f) return "condensing";
    return "clean-air";
}
