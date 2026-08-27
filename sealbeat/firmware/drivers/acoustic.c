/*
 * SealBeat acoustic driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include "acoustic.h"

static float cycle_pattern(uint32_t minute)
{
    switch (minute % 8u) {
    case 0u: return 0.10f;
    case 1u: return 0.24f;
    case 2u: return 0.62f;
    case 3u: return 0.18f;
    case 4u: return 0.42f;
    case 5u: return 0.55f;
    case 6u: return 0.16f;
    default: return 0.08f;
    }
}

void acoustic_init(acoustic_state_t *state, appliance_profile_t profile)
{
    state->latch_sharpness = sb_profile_bias(profile, 0.58f, 0.64f, 0.60f);
    state->compressor_harmonic = sb_profile_bias(profile, 0.40f, 0.46f, 0.44f);
    state->frame_vibration = sb_profile_bias(profile, 0.18f, 0.22f, 0.14f);
    state->slam_energy = 0.12f;
    state->noise_floor = 0.08f;
    state->compressor_burden = 0.26f;
    state->shock_events = 0u;
}

void acoustic_sample(acoustic_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door, const seal_state_t *seal)
{
    const float pattern = cycle_pattern(minute);
    const float burden_gain = 1.0f - seal->closure_confidence;
    state->latch_sharpness = sb_clampf(0.45f + pattern * 0.30f + (door->close_velocity * 0.18f) - door->bounce_count * 0.07f, 0.05f, 1.0f);
    state->compressor_harmonic = sb_clampf(sb_profile_bias(profile, 0.38f, 0.48f, 0.42f) + burden_gain * 0.40f + minute * 0.003f, 0.0f, 1.0f);
    state->frame_vibration = sb_clampf(0.08f + door->hinge_skew * 0.45f + pattern * 0.15f, 0.0f, 1.0f);
    state->slam_energy = sb_clampf(door->close_velocity * 0.55f + pattern * 0.22f, 0.0f, 1.0f);
    state->noise_floor = sb_clampf(0.06f + sb_profile_bias(profile, 0.02f, 0.04f, 0.01f) + (float)(minute % 5u) * 0.01f, 0.0f, 1.0f);
    state->compressor_burden = sb_clampf(0.20f + burden_gain * 0.60f + door->dwell_open_seconds / 120.0f, 0.0f, 1.0f);
    if (door->bounce_count > 0.25f || state->slam_energy > 0.64f) {
        state->shock_events += 1u;
    }
}

const char *acoustic_label(const acoustic_state_t *state)
{
    if (state->compressor_burden > 0.70f) return "heavy-recovery";
    if (state->frame_vibration > 0.42f) return "hinge-rattle";
    if (state->latch_sharpness < 0.32f) return "soft-latch";
    return "nominal";
}
