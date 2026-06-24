/*
 * harmonics.c — IEC 61000-4-7 harmonic extraction and THD
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Extracts harmonic magnitudes and phases from the FFT output according to
 * IEC 61000-4-7.  The standard specifies 50 Hz (5 Hz groups) or 60 Hz (5 Hz
 * groups) with grouping of adjacent bins.  At 2048 Sa/s with a 2048-point FFT,
 * the bin width is 1 Hz — so each harmonic (at integer multiples of the
 * fundamental) falls on a single bin, and IEC grouping sums the ±2 adjacent
 * bins (5 Hz group width).
 *
 * THD is computed as:
 *   THD = sqrt(Σ H_n^2) / H_1   for n = 2..50
 */

#include "harmonics.h"
#include "dsp_fft.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float bin_width;  /* Hz per FFT bin */

void harmonics_init(void) {
    bin_width = (float)SAMPLE_RATE_HZ / (float)FFT_SIZE;  /* 1.0 Hz */
}

/* ========================================================================
 * Find the FFT bin index closest to a given frequency
 * ======================================================================== */
static int freq_to_bin(float freq) {
    int bin = (int)(freq / bin_width + 0.5f);
    if (bin < 0) bin = 0;
    if (bin >= FFT_SIZE / 2) bin = FFT_SIZE / 2 - 1;
    return bin;
}

/* ========================================================================
 * Extract grouped harmonic magnitude (IEC 61000-4-7: sum ±2 bins)
 * ======================================================================== */
static float grouped_magnitude(const float *fft_out, int center_bin) {
    float sum_sq = 0.0f;
    for (int k = center_bin - 2; k <= center_bin + 2; k++) {
        if (k >= 0 && k < FFT_SIZE / 2) {
            float re = fft_out[k * 2];
            float im = fft_out[k * 2 + 1];
            float mag = sqrtf(re * re + im * im);
            sum_sq += mag * mag;
        }
    }
    return sqrtf(sum_sq);
}

/* ========================================================================
 * Extract harmonic phase at a specific bin
 * ======================================================================== */
static float bin_phase(const float *fft_out, int bin) {
    float re = fft_out[bin * 2];
    float im = fft_out[bin * 2 + 1];
    return atan2f(im, re);
}

/* ========================================================================
 * Extract voltage harmonics for one phase
 * ======================================================================== */
void harmonics_extract_v(const float *fft_out, int phase,
                         harmonic_data_t *h, float grid_freq) {
    for (int order = 0; order <= HARMONIC_MAX_ORDER; order++) {
        float freq = grid_freq * (float)order;
        int bin = freq_to_bin(freq);

        /* Apply Hann window amplitude correction factor (×2 for coherent gain) */
        float mag = grouped_magnitude(fft_out, bin) * 2.0f / (float)(FFT_SIZE / 2);
        h->v_mag[phase][order] = mag;
        h->v_ph[phase][order] = bin_phase(fft_out, bin);
    }
}

/* ========================================================================
 * Extract current harmonics for one phase
 * ======================================================================== */
void harmonics_extract_i(const float *fft_out, int phase,
                         harmonic_data_t *h, float grid_freq) {
    for (int order = 0; order <= HARMONIC_MAX_ORDER; order++) {
        float freq = grid_freq * (float)order;
        int bin = freq_to_bin(freq);

        float mag = grouped_magnitude(fft_out, bin) * 2.0f / (float)(FFT_SIZE / 2);
        h->i_mag[phase][order] = mag;
        h->i_ph[phase][order] = bin_phase(fft_out, bin);
    }
}

/* ========================================================================
 * Compute Total Harmonic Distortion (THD)
 *
 * THD = sqrt( Σ_{n=2}^{50} H_n^2 ) / H_1 × 100%
 *
 * where H_n is the RMS magnitude of the nth harmonic.
 * The FFT magnitude is amplitude (peak), so divide by sqrt(2) for RMS,
 * but since it appears in both numerator and denominator, the factor cancels.
 * ======================================================================== */
void harmonics_compute_thd(const harmonic_data_t *h, power_metrics_t *m) {
    for (int phase = 0; phase < 3; phase++) {
        float fundamental = h->v_mag[phase][1];
        if (fundamental > 0.001f) {
            float sum_sq = 0.0f;
            for (int n = 2; n <= HARMONIC_MAX_ORDER; n++) {
                sum_sq += h->v_mag[phase][n] * h->v_mag[phase][n];
            }
            m->thd_v[phase] = sqrtf(sum_sq) / fundamental * 100.0f;
        } else {
            m->thd_v[phase] = 0.0f;
        }

        fundamental = h->i_mag[phase][1];
        if (fundamental > 0.001f) {
            float sum_sq = 0.0f;
            for (int n = 2; n <= HARMONIC_MAX_ORDER; n++) {
                sum_sq += h->i_mag[phase][n] * h->i_mag[phase][n];
            }
            m->thd_i[phase] = sqrtf(sum_sq) / fundamental * 100.0f;
        } else {
            m->thd_i[phase] = 0.0f;
        }
    }
}