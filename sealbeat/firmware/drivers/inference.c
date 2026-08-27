/*
 * SealBeat inference engine
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include "inference.h"

void inference_init(inference_state_t *state)
{
    state->seal_integrity = 0.90f;
    state->safety_confidence = 0.92f;
    state->hinge_wear = 0.08f;
    state->maintenance_priority = 0.08f;
    state->energy_penalty = 0.05f;
    state->closure_quality = 0.90f;
    state->service_score = 0.90f;
    state->alert = ALERT_NONE;
}

static alert_level_t decide_alert(const inference_state_t *state, appliance_profile_t profile)
{
    const float critical_limit = profile == APPLIANCE_PROFILE_PHARMACY_COOLER ? 0.72f : 0.84f;
    if (state->safety_confidence < 0.42f || state->maintenance_priority > 0.88f) return ALERT_CRITICAL;
    if (state->safety_confidence < 0.58f || state->seal_integrity < critical_limit - 0.24f || state->hinge_wear > 0.66f) return ALERT_WARNING;
    if (state->maintenance_priority > 0.42f || state->energy_penalty > 0.34f) return ALERT_CAUTION;
    if (state->seal_integrity < critical_limit || state->closure_quality < 0.72f) return ALERT_INFO;
    return ALERT_NONE;
}

void inference_update(inference_state_t *state, const appliance_snapshot_t *current, const appliance_snapshot_t *previous, appliance_profile_t profile)
{
    const float edge_mean = (current->seal.top_edge_score + current->seal.latch_edge_score + current->seal.bottom_edge_score + current->seal.hinge_edge_score) / 4.0f;
    const float rebound_penalty = current->thermal.warm_rebound_c * 0.10f;
    const float recovery_penalty = current->thermal.recovery_tau_s / 220.0f;
    const float bounce_penalty = current->door.bounce_count * 0.22f;
    const float drift_penalty = current->door.hinge_skew * 0.42f;
    const float pharmacy_bias = profile == APPLIANCE_PROFILE_PHARMACY_COOLER ? 0.06f : 0.0f;

    state->seal_integrity = sb_clampf(edge_mean * 0.55f + current->seal.compression_uniformity * 0.30f + current->seal.magnetic_pull * 0.15f - pharmacy_bias, 0.0f, 1.0f);
    state->closure_quality = sb_clampf(current->seal.closure_confidence * 0.55f + current->acoustic.latch_sharpness * 0.15f + (1.0f - bounce_penalty) * 0.15f + (1.0f - current->seal.final_gap_mm / 6.0f) * 0.15f, 0.0f, 1.0f);
    state->hinge_wear = sb_clampf(current->door.hinge_skew * 0.65f + current->acoustic.frame_vibration * 0.20f + current->door.tilt_drift_deg / 8.0f, 0.0f, 1.0f);
    state->energy_penalty = sb_clampf((1.0f - current->seal.closure_confidence) * 0.35f + current->thermal.recovery_tau_s / 260.0f + current->acoustic.compressor_burden * 0.25f, 0.0f, 1.0f);
    state->safety_confidence = sb_clampf(current->thermal.safety_margin * 0.56f + state->seal_integrity * 0.25f + (1.0f - rebound_penalty) * 0.12f + (1.0f - recovery_penalty) * 0.07f - (profile == APPLIANCE_PROFILE_PHARMACY_COOLER ? current->thermal.warm_rebound_c * 0.04f : 0.0f), 0.0f, 1.0f);
    state->maintenance_priority = sb_clampf((1.0f - state->seal_integrity) * 0.32f + state->hinge_wear * 0.25f + state->energy_penalty * 0.23f + (1.0f - state->safety_confidence) * 0.20f + drift_penalty * 0.12f, 0.0f, 1.0f);
    state->service_score = sb_clampf(1.0f - state->maintenance_priority * 0.65f - (1.0f - state->safety_confidence) * 0.20f + (previous->inference.service_score - 0.5f) * 0.05f, 0.0f, 1.0f);
    state->alert = decide_alert(state, profile);
}

const char *inference_primary_reason(const appliance_snapshot_t *snapshot)
{
    if (snapshot->inference.safety_confidence < 0.58f) return "thermal-safety-risk";
    if (snapshot->inference.hinge_wear > 0.48f) return "hinge-drift";
    if (snapshot->seal.top_edge_score < 0.55f) return "top-seal-loss";
    if (snapshot->seal.bottom_edge_score < 0.55f) return "bottom-seal-loss";
    if (snapshot->inference.energy_penalty > 0.36f) return "recovery-energy-penalty";
    return "nominal-operation";
}
