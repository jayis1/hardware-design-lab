/*
 * dsp.h — DSP functions for tremor analysis
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#ifndef TREMORSENSE_DSP_H
#define TREMORSENSE_DSP_H

#include <stdint.h>
#include <math.h>

/* ---- Butterworth bandpass filter state (4th order) ---- */
typedef struct {
    float a1[8];       /* Denominator coefficients (4 sections × 2) */
    float a2[8];
    float b0[4];        /* Numerator coefficients (4 sections) */
    float b1[4];
    float b2[4];
    float x1[4];        /* Previous input samples per section */
    float x2[4];
    float y1[4];        /* Previous output samples per section */
    float y2[4];
    int   sections;     /* Number of biquad sections (2 for 4th order) */
} butter_state_t;

/* Initialize Butterworth bandpass filter */
void butter_bandpass_init(butter_state_t *state, float low_hz, float high_hz,
                          float sample_rate_hz, int order);

/* Apply Butterworth bandpass filter to input signal */
void butter_bandpass_apply(butter_state_t *state, const float *input,
                           float *output, int n_samples);

/* ---- FFT (radix-2 Cooley-Tukey) ---- */
/* Computes in-place FFT of size N (must be power of 2)
 * real[], imag[] are input/output arrays of size N
 */
void fft_compute(float *real, float *imag, int n);

/* ---- Window functions ---- */
void fft_hanning_window(float *data, int n);
void fft_hamming_window(float *data, int n);
void fft_blackman_window(float *data, int n);

/* ---- Utility DSP functions ---- */
float compute_rms(const float *data, int n);
float compute_mean(const float *data, int n);
float compute_variance(const float *data, int n, float mean);
float compute_kurtosis(const float *data, int n, float mean, float variance);
float compute_hjorth_activity(const float *data, int n);
float compute_hjorth_mobility(const float *data, int n);
float compute_hjorth_complexity(const float *data, int n);
float compute_spectral_centroid(const float *psd, int n_bins, float freq_res);
float compute_spectral_entropy(const float *psd, int n_bins, float total_power);
float compute_harmonic_ratio(const float *psd, int n_bins, int fund_bin,
                              int harmonic);

/* ---- Peak detection ---- */
typedef struct {
    int   bin_index;
    float frequency_hz;
    float power;
    float quality_factor;  /* Sharpness of peak */
} spectral_peak_t;

int find_dominant_peak(const float *psd, int bin_low, int bin_high,
                       spectral_peak_t *peak);

/* ---- Watchdog feed (provided by BSP) ---- */
void watchdog_feed(void);

#endif /* TREMORSENSE_DSP_H */