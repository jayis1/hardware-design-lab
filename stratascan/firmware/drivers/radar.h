/*
 * radar.h — Range FFT & Synthetic-Aperture Migration
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_RADAR_H
#define STRATASCAN_RADAR_H

#include <stdint.h>
#include "../board.h"

void radar_init(void);
void radar_range_fft(const float *I, const float *Q, float *out,
                     uint16_t n, const band_preset_t *bp);
void radar_compute_depth_axis(float *depth, uint16_t n,
                               const band_preset_t *bp, float eps_r);
void radar_background_subtract(float *bscan, uint16_t width,
                                uint16_t height);
void radar_auto_gain_control(float *ascan, uint16_t n, float target_rms);
void radar_synthetic_aperture_focus(float *bscan, uint16_t width,
                                     uint16_t height, float dx,
                                     float eps_r);

#endif /* STRATASCAN_RADAR_H */