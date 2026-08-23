/*
 * acoustic.h — StudGuard wall acoustic sensing interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_ACOUSTIC_H
#define STUDGUARD_ACOUSTIC_H

#include <stddef.h>
#include <stdint.h>
#include "../board.h"

typedef struct {
    float samples[AUDIO_FRAME_SAMPLES];
    size_t count;
    float energy;
    float decay_ms;
    float centroid_hz;
    float phase_stability;
    float damping_ratio;
} acoustic_frame_t;

typedef struct {
    float baseline_energy;
    float baseline_decay_ms;
    float baseline_centroid_hz;
    float baseline_phase_stability;
    float baseline_damping_ratio;
} acoustic_baseline_t;

void acoustic_init(void);
void acoustic_capture_frame(acoustic_frame_t *frame, uint32_t tick, float wetness_hint, float wall_temp_c);
void acoustic_update_baseline(acoustic_baseline_t *baseline, const acoustic_frame_t *frame, uint32_t baseline_count);
float acoustic_compare_to_baseline(const acoustic_baseline_t *baseline, const acoustic_frame_t *frame, float *confidence);
float acoustic_peer_attenuation(const acoustic_frame_t *local, const acoustic_frame_t *peer);
void acoustic_debug_summary(const acoustic_frame_t *frame, char *buffer, size_t buffer_len);

#endif
