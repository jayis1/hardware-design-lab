/*
 * harmonics.h — IEC 61000-4-7 harmonic analysis
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_HARMONICS_H
#define WATTLENS_HARMONICS_H

#include "board.h"

void harmonics_init(void);
void harmonics_extract_v(const float *fft_out, int phase,
                         harmonic_data_t *h, float grid_freq);
void harmonics_extract_i(const float *fft_out, int phase,
                         harmonic_data_t *h, float grid_freq);
void harmonics_compute_thd(const harmonic_data_t *h, power_metrics_t *m);

#endif /* WATTLENS_HARMONICS_H */