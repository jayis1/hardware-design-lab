/*
 * SealBeat thermal recovery driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "thermal.h"

void thermal_init(thermal_state_t *state, appliance_profile_t profile)
{
    state->ambient_temp_c = sb_profile_bias(profile, 23.5f, 28.0f, 21.0f);
    state->humidity_rh = sb_profile_bias(profile, 51.0f, 56.0f, 45.0f);
    state->edge_temp_c = sb_profile_bias(profile, 4.5f, -15.0f, 4.0f);
    state->compartment_temp_c = sb_profile_bias(profile, 3.6f, -18.5f, 4.6f);
    state->warm_rebound_c = 0.5f;
    state->recovery_tau_s = sb_profile_bias(profile, 48.0f, 75.0f, 42.0f);
    state->frost_risk = sb_profile_bias(profile, 0.10f, 0.28f, 0.05f);
    state->safety_margin = sb_profile_bias(profile, 0.78f, 0.82f, 0.92f);
}

void thermal_sample(thermal_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door, const seal_state_t *seal)
{
    const float intrusion = (1.0f - seal->closure_confidence) * 4.0f + door->dwell_open_seconds / 24.0f;
    const float traffic = (float)(minute % 6u) * 0.15f;
    state->ambient_temp_c = sb_profile_bias(profile, 23.0f, 27.0f, 21.5f) + (float)(minute % 5u) * 0.4f;
    state->humidity_rh = sb_clampf(sb_profile_bias(profile, 52.0f, 58.0f, 44.0f) + traffic * 4.0f + seal->final_gap_mm * 1.6f, 20.0f, 95.0f);
    state->edge_temp_c = sb_profile_bias(profile, 4.2f, -15.5f, 4.3f) + intrusion * sb_profile_bias(profile, 0.6f, 0.9f, 0.4f);
    state->compartment_temp_c = sb_profile_bias(profile, 3.7f, -18.0f, 4.5f) + intrusion * sb_profile_bias(profile, 0.35f, 0.45f, 0.28f);
    state->warm_rebound_c = sb_clampf(0.3f + intrusion * 0.9f + (minute > 32u ? 0.5f : 0.0f), 0.0f, 8.0f);
    state->recovery_tau_s = sb_clampf(sb_profile_bias(profile, 46.0f, 72.0f, 40.0f) + intrusion * 26.0f + door->bounce_count * 18.0f, 8.0f, 240.0f);
    state->frost_risk = sb_clampf(sb_profile_bias(profile, 0.08f, 0.24f, 0.04f) + (state->humidity_rh / 100.0f) * 0.16f + (profile == APPLIANCE_PROFILE_UPRIGHT_FREEZER ? seal->final_gap_mm * 0.04f : 0.0f), 0.0f, 1.0f);
    state->safety_margin = sb_clampf(sb_profile_bias(profile, 0.84f, 0.80f, 0.95f) - state->warm_rebound_c * 0.06f - state->recovery_tau_s / 400.0f, 0.0f, 1.0f);
}

const char *thermal_status_label(const thermal_state_t *state)
{
    if (state->safety_margin < 0.55f) return "safety-drift";
    if (state->frost_risk > 0.42f) return "frost-prone";
    if (state->warm_rebound_c > 1.8f) return "warm-rebound";
    return "stable";
}
