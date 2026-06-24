/*
 * dsp_fft.h — CMSIS-DSP FFT wrapper
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_DSP_FFT_H
#define WATTLENS_DSP_FFT_H

#include "board.h"

void dsp_fft_init(void);
void dsp_fft_run(float *input, float *output);
void dsp_fft_magnitude(float *fft_out, float *mag, int n_bins);

#endif /* WATTLENS_DSP_FFT_H */