/*
 * mesh.c — StudGuard peer correlation and origin-band solving
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "mesh.h"
#include "../registers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float peer_noise(uint32_t tick, uint32_t index) {
    uint32_t x = tick + 37u * index + 0xA341316Cu;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (float)(x & 0xFFFFu) / 65535.0f;
}

void mesh_init(mesh_state_t *state, uint8_t self_id) {
    size_t i;
    memset(state, 0, sizeof(*state));
    for (i = 0; i < MAX_MESH_NODES; ++i) {
        state->peers[i].id = (uint8_t)(i == 0 ? self_id : i + 1u);
        state->peers[i].x = (float)(i % 2u) * 0.28f;
        state->peers[i].y = 0.0f;
        state->peers[i].z = (float)i * 0.34f;
    }
    SG_UWB.CTRL = 1u;
}

void mesh_synthesize_peers(mesh_state_t *state, uint32_t tick, float local_leak, float local_spread, float local_confidence) {
    size_t i;
    state->count = 4u;
    for (i = 0; i < state->count; ++i) {
        float distance_factor = 1.0f - 0.13f * (float)i;
        float vertical_gradient = 0.12f * sinf((float)tick * 0.00033f + (float)i * 0.8f);
        float noise = (peer_noise(tick, (uint32_t)i) - 0.5f) * 0.08f;
        state->peers[i].id = (uint8_t)(i + 1u);
        state->peers[i].x = 0.0f;
        state->peers[i].y = 0.0f;
        state->peers[i].z = 0.30f * (float)i;
        state->peers[i].leak_activity = clampf(local_leak * distance_factor + vertical_gradient + noise, 0.0f, 1.0f);
        state->peers[i].wetness_spread = clampf(local_spread * (0.9f - 0.08f * (float)i) + fabsf(vertical_gradient), 0.0f, 1.0f);
        state->peers[i].confidence = clampf(local_confidence - 0.05f * (float)i, 0.2f, 0.98f);
        state->peers[i].attenuation = clampf(0.1f + 0.15f * (float)i + vertical_gradient, -1.0f, 1.0f);
        state->peers[i].cap_mean = clampf(0.15f + local_spread * distance_factor + noise, 0.0f, 1.0f);
    }
    SG_UWB.STATUS = 1u;
    SG_UWB.TIMESTAMP_LO = tick;
}

void mesh_integrate_measurement(mesh_state_t *state, sg_measurement_t *measurement) {
    size_t i;
    float leak_sum = measurement->leak_activity;
    float spread_sum = measurement->wetness_spread;
    float conf_sum = measurement->confidence;
    float weighted_origin = 0.0f;
    float weight_sum = 0.0f;

    for (i = 0; i < state->count; ++i) {
        float weight = 0.5f + state->peers[i].leak_activity + state->peers[i].wetness_spread;
        leak_sum += state->peers[i].leak_activity;
        spread_sum += state->peers[i].wetness_spread;
        conf_sum += state->peers[i].confidence;
        weighted_origin += state->peers[i].z * weight;
        weight_sum += weight;
    }

    state->consensus_leak = leak_sum / (float)(state->count + 1u);
    state->consensus_spread = spread_sum / (float)(state->count + 1u);
    state->consensus_confidence = conf_sum / (float)(state->count + 1u);
    state->origin_band = weight_sum > 1e-6f ? weighted_origin / weight_sum : 0.0f;

    measurement->peer_count = (uint8_t)state->count;
    measurement->origin_band = clampf(state->origin_band, 0.0f, 2.0f);
    measurement->peer_attenuation = state->count ? state->peers[0].attenuation : 0.0f;
    measurement->leak_activity = clampf(0.62f * measurement->leak_activity + 0.38f * state->consensus_leak, 0.0f, 1.0f);
    measurement->wetness_spread = clampf(0.60f * measurement->wetness_spread + 0.40f * state->consensus_spread, 0.0f, 1.0f);
    measurement->confidence = clampf(0.55f * measurement->confidence + 0.45f * state->consensus_confidence, 0.0f, 1.0f);
}

void mesh_summary(const mesh_state_t *state, char *buffer, size_t buffer_len) {
    if (buffer == NULL || buffer_len == 0u) {
        return;
    }
    snprintf(buffer, buffer_len,
             "peers=%zu consensus(leak=%.3f spread=%.3f conf=%.3f) origin=%.2fm",
             state->count,
             state->consensus_leak,
             state->consensus_spread,
             state->consensus_confidence,
             state->origin_band);
}
