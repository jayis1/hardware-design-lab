/*
 * SealBeat seal fusion driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "seal.h"

void seal_init(seal_state_t *state, appliance_profile_t profile)
{
    state->top_edge_score = sb_profile_bias(profile, 0.86f, 0.82f, 0.91f);
    state->latch_edge_score = sb_profile_bias(profile, 0.88f, 0.84f, 0.93f);
    state->bottom_edge_score = sb_profile_bias(profile, 0.83f, 0.79f, 0.89f);
    state->hinge_edge_score = sb_profile_bias(profile, 0.87f, 0.83f, 0.92f);
    state->compression_uniformity = 0.84f;
    state->magnetic_pull = 0.76f;
    state->final_gap_mm = 0.4f;
    state->leak_vector = 0.08f;
    state->closure_confidence = 0.88f;
}

void seal_sample(seal_state_t *state, appliance_profile_t profile, uint32_t minute, const door_state_t *door)
{
    const float wear = minute * sb_profile_bias(profile, 0.004f, 0.005f, 0.003f);
    const float top_penalty = (minute > 20u ? 0.06f : 0.0f) + door->bounce_count * 0.10f;
    const float bottom_penalty = (door->dwell_open_seconds > 16.0f ? 0.04f : 0.0f) + (minute > 30u ? 0.05f : 0.0f);
    const float hinge_penalty = door->hinge_skew * 0.18f;
    const float latch_penalty = door->close_velocity > 0.55f ? 0.05f : 0.02f;

    state->top_edge_score = sb_clampf(sb_profile_bias(profile, 0.88f, 0.84f, 0.93f) - wear - top_penalty, 0.0f, 1.0f);
    state->latch_edge_score = sb_clampf(sb_profile_bias(profile, 0.90f, 0.86f, 0.95f) - wear * 0.8f - latch_penalty, 0.0f, 1.0f);
    state->bottom_edge_score = sb_clampf(sb_profile_bias(profile, 0.85f, 0.81f, 0.90f) - wear * 1.1f - bottom_penalty, 0.0f, 1.0f);
    state->hinge_edge_score = sb_clampf(sb_profile_bias(profile, 0.89f, 0.85f, 0.94f) - wear * 0.9f - hinge_penalty, 0.0f, 1.0f);
    state->compression_uniformity = sb_clampf((state->top_edge_score + state->latch_edge_score + state->bottom_edge_score + state->hinge_edge_score) / 4.0f - door->bounce_count * 0.12f, 0.0f, 1.0f);
    state->magnetic_pull = sb_clampf(0.68f + state->latch_edge_score * 0.20f - door->hinge_skew * 0.12f, 0.0f, 1.0f);
    state->final_gap_mm = sb_clampf(0.25f + (1.0f - state->compression_uniformity) * 2.8f + door->bounce_count * 1.1f, 0.0f, 6.0f);
    state->leak_vector = sb_clampf((state->top_edge_score - state->bottom_edge_score) * 0.5f + (state->hinge_edge_score - state->latch_edge_score) * 0.5f + 0.5f, 0.0f, 1.0f);
    state->closure_confidence = sb_clampf((state->compression_uniformity * 0.45f) + (state->magnetic_pull * 0.35f) + ((6.0f - state->final_gap_mm) / 6.0f) * 0.20f, 0.0f, 1.0f);
}

const char *seal_edge_label(const seal_state_t *state)
{
    if (state->top_edge_score < 0.55f) return "top-edge";
    if (state->bottom_edge_score < 0.55f) return "bottom-edge";
    if (state->hinge_edge_score < 0.55f) return "hinge-edge";
    if (state->latch_edge_score < 0.55f) return "latch-edge";
    return "balanced";
}
