/*
 * acoustic.c — StudGuard wall acoustic sensing simulation
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "acoustic.h"
#include "../registers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static float prng_unit(uint32_t index) {
    uint32_t x = 1103515245u * (index + 12345u) + 1013904223u;
    x ^= x >> 11;
    x ^= x << 7;
    x ^= x >> 13;
    return (float)(x & 0xFFFFu) / 65535.0f;
}

void acoustic_init(void) {
    SG_PIEZO.CTRL = 1u;
    SG_PIEZO.PERIOD = 96u;
    SG_PIEZO.DUTY = 32u;
    SG_PIEZO.GAIN = 12u;
    SG_ADC.CTRL = 1u;
    SG_ADC.SAMPLE_RATE = AUDIO_SAMPLE_RATE_HZ;
}

static void derive_features(acoustic_frame_t *frame) {
    float total_energy = 0.0f;
    float weighted_freq = 0.0f;
    float weighted_mag = 0.0f;
    float late_energy = 0.0f;
    float early_energy = 0.0f;
    float zero_crossings = 0.0f;
    float prev = frame->samples[0];
    size_t i;

    for (i = 0; i < frame->count; ++i) {
        float s = frame->samples[i];
        float energy = s * s;
        float ratio = (float)i / (float)(frame->count ? frame->count : 1u);
        float pseudo_freq = 350.0f + ratio * 7650.0f;
        total_energy += energy;
        weighted_freq += pseudo_freq * fabsf(s);
        weighted_mag += fabsf(s);
        if (i < frame->count / 3u) {
            early_energy += energy;
        } else {
            late_energy += energy;
        }
        if ((s >= 0.0f && prev < 0.0f) || (s < 0.0f && prev >= 0.0f)) {
            zero_crossings += 1.0f;
        }
        prev = s;
    }

    frame->energy = total_energy / (float)(frame->count ? frame->count : 1u);
    frame->centroid_hz = weighted_mag > 1e-6f ? weighted_freq / weighted_mag : 0.0f;
    frame->phase_stability = clampf(1.0f - (zero_crossings / (float)frame->count), 0.0f, 1.0f);

    if (late_energy > 1e-6f) {
        frame->decay_ms = 6.0f + 24.0f * logf((early_energy + 1e-6f) / (late_energy + 1e-6f));
    } else {
        frame->decay_ms = 36.0f;
    }
    frame->decay_ms = clampf(frame->decay_ms, 4.0f, 48.0f);
    frame->damping_ratio = clampf(late_energy / (early_energy + late_energy + 1e-6f), 0.02f, 0.98f);
}

void acoustic_capture_frame(acoustic_frame_t *frame, uint32_t tick, float wetness_hint, float wall_temp_c) {
    size_t i;
    float wet = clampf(wetness_hint, 0.0f, 1.0f);
    float decay_factor = 0.010f + wet * 0.012f;
    float resonance_shift = (22.0f - wall_temp_c) * 0.02f + wet * -0.09f;
    float phase_noise = 0.04f + wet * 0.08f;

    memset(frame, 0, sizeof(*frame));
    frame->count = AUDIO_FRAME_SAMPLES;

    for (i = 0; i < frame->count; ++i) {
        float t = (float)i / (float)AUDIO_SAMPLE_RATE_HZ;
        float chirp = sinf(2.0f * (float)M_PI * (380.0f + 5200.0f * t) * t);
        float resonance = 0.55f * sinf(2.0f * (float)M_PI * (1850.0f + resonance_shift * 800.0f) * t);
        float cavity = 0.28f * cosf(2.0f * (float)M_PI * (620.0f + wet * 140.0f) * t);
        float envelope = expf(-(float)i * decay_factor);
        float noise = (prng_unit(tick + (uint32_t)i * 17u) - 0.5f) * phase_noise;
        float sample = envelope * (chirp + resonance + cavity) + noise;
        frame->samples[i] = sample;
        SG_ADC.FIFO[i] = (int16_t)(sample * 2048.0f);
    }

    SG_ADC.FIFO_LEVEL = frame->count;
    derive_features(frame);
    SG_PIEZO.STATUS = 1u;
    SG_ADC.STATUS = 1u;
}

void acoustic_update_baseline(acoustic_baseline_t *baseline, const acoustic_frame_t *frame, uint32_t baseline_count) {
    float n = (float)baseline_count;
    if (baseline_count == 0u) {
        baseline->baseline_energy = frame->energy;
        baseline->baseline_decay_ms = frame->decay_ms;
        baseline->baseline_centroid_hz = frame->centroid_hz;
        baseline->baseline_phase_stability = frame->phase_stability;
        baseline->baseline_damping_ratio = frame->damping_ratio;
        return;
    }

    baseline->baseline_energy = (baseline->baseline_energy * n + frame->energy) / (n + 1.0f);
    baseline->baseline_decay_ms = (baseline->baseline_decay_ms * n + frame->decay_ms) / (n + 1.0f);
    baseline->baseline_centroid_hz = (baseline->baseline_centroid_hz * n + frame->centroid_hz) / (n + 1.0f);
    baseline->baseline_phase_stability = (baseline->baseline_phase_stability * n + frame->phase_stability) / (n + 1.0f);
    baseline->baseline_damping_ratio = (baseline->baseline_damping_ratio * n + frame->damping_ratio) / (n + 1.0f);
}

float acoustic_compare_to_baseline(const acoustic_baseline_t *baseline, const acoustic_frame_t *frame, float *confidence) {
    float energy_shift = fabsf(frame->energy - baseline->baseline_energy) / (baseline->baseline_energy + 1e-6f);
    float decay_shift = fabsf(frame->decay_ms - baseline->baseline_decay_ms) / (baseline->baseline_decay_ms + 1e-6f);
    float centroid_shift = fabsf(frame->centroid_hz - baseline->baseline_centroid_hz) / (baseline->baseline_centroid_hz + 1e-6f);
    float phase_shift = fabsf(frame->phase_stability - baseline->baseline_phase_stability);
    float damping_shift = fabsf(frame->damping_ratio - baseline->baseline_damping_ratio);
    float activity = 0.34f * energy_shift + 0.22f * decay_shift + 0.18f * centroid_shift + 0.10f * phase_shift + 0.16f * damping_shift;

    activity = clampf(activity, 0.0f, 1.0f);
    if (confidence != NULL) {
        *confidence = clampf(0.92f - 0.45f * phase_shift - 0.15f * energy_shift, 0.1f, 0.99f);
    }
    return activity;
}

float acoustic_peer_attenuation(const acoustic_frame_t *local, const acoustic_frame_t *peer) {
    float energy_ratio = peer->energy / (local->energy + 1e-6f);
    float decay_ratio = peer->decay_ms / (local->decay_ms + 1e-6f);
    float centroid_ratio = peer->centroid_hz / (local->centroid_hz + 1e-6f);
    float combined = 0.50f * energy_ratio + 0.25f * decay_ratio + 0.25f * centroid_ratio;
    return clampf(1.0f - combined, -1.0f, 1.0f);
}

void acoustic_debug_summary(const acoustic_frame_t *frame, char *buffer, size_t buffer_len) {
    if (buffer == NULL || buffer_len == 0u) {
        return;
    }
    snprintf(buffer, buffer_len,
             "energy=%.4f decay=%.2fms centroid=%.1fHz phase=%.3f damp=%.3f",
             frame->energy,
             frame->decay_ms,
             frame->centroid_hz,
             frame->phase_stability,
             frame->damping_ratio);
}
