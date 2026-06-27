/*
 * dsp.c — DSP implementation for tremor analysis
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Implements: Butterworth bandpass filter (4th order, biquad cascade),
 * radix-2 FFT, window functions, and spectral feature extraction.
 */

#include "dsp.h"
#include "../board.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* ---- Butterworth bandpass filter ---- */

void butter_bandpass_init(butter_state_t *state, float low_hz, float high_hz,
                          float sample_rate_hz, int order)
{
    memset(state, 0, sizeof(*state));

    /* For a 4th-order Butterworth bandpass, we use 2 biquad sections.
     * Coefficients computed via bilinear transform of 2nd-order
     * Butterworth lowpass + highpass prototypes.
     *
     * For simplicity (and to avoid linking a full filter design library
     * on the MCU), we pre-compute the biquad coefficients for the
     * 3.5–12 Hz band at 1000 Hz sample rate. In production, these
     * would be computed at runtime or loaded from calibration.
     */

    float fs = sample_rate_hz;
    float f_low = low_hz;
    float f_high = high_hz;

    /* Pre-warped frequencies */
    float w_low = 2.0f * fs * tanf(M_PI * f_low / fs);
    float w_high = 2.0f * fs * tanf(M_PI * f_high / fs);
    float bw = w_high - w_low;
    float w0 = sqrtf(w_low * w_high);

    /* For each biquad section (2 sections for 4th order) */
    state->sections = order / 2;  /* 2 for 4th order */

    /* Section 1: lowpass part of bandpass */
    float Q1 = 0.5412f;  /* Butterworth Q for 4th order */
    float a = bw / (Q1 * w0);
    float b = w0 / (Q1 * w0);

    /* Bilinear transform coefficients (s-domain → z-domain) */
    float k = 2.0f * fs;
    float a0 = bw * bw;
    float a1 = 0.0f;
    float a2 = -bw * bw;

    /* Simplified coefficient computation for bandpass biquad */
    for (int i = 0; i < state->sections && i < 4; i++) {
        /* Standard bandpass biquad coefficients */
        float alpha = sinf(2.0f * M_PI * (i == 0 ? f_low : f_high) / fs) /
                      (2.0f * Q1);

        float cosw0 = cosf(2.0f * M_PI * (i == 0 ? f_low : f_high) / fs);

        /* Bandpass (constant 0 dB peak gain) */
        state->b0[i] = alpha / (1.0f + alpha);
        state->b1[i] = 0.0f;
        state->b2[i] = -alpha / (1.0f + alpha);
        state->a1[2 * i] = -2.0f * cosw0 / (1.0f + alpha);
        state->a2[2 * i] = (1.0f - alpha) / (1.0f + alpha);

        state->x1[i] = state->x2[i] = 0.0f;
        state->y1[i] = state->y2[i] = 0.0f;
    }
}

void butter_bandpass_apply(butter_state_t *state, const float *input,
                           float *output, int n_samples)
{
    /* Cascade of biquad sections (Direct Form II Transposed) */
    float temp[FFT_SIZE];  /* Intermediate buffer between sections */

    /* First pass through all sections */
    const float *src = input;
    float *dst = (state->sections > 1) ? temp : output;

    for (int s = 0; s < state->sections; s++) {
        for (int i = 0; i < n_samples; i++) {
            float x = src[i];
            float y = state->b0[s] * x + state->b1[s] * state->x1[s]
                     + state->b2[s] * state->x2[s]
                     - state->a1[2 * s] * state->y1[s]
                     - state->a2[2 * s] * state->y2[s];

            state->x2[s] = state->x1[s];
            state->x1[s] = x;
            state->y2[s] = state->y1[s];
            state->y1[s] = y;

            dst[i] = y;
        }

        /* Next section reads from this section's output */
        src = dst;
        dst = (s + 1 < state->sections) ? temp : output;
    }

    /* If multiple sections, final output is already in 'output' */
}

/* ---- Radix-2 Cooley-Tukey FFT ---- */

static void fft_bit_reverse(float *real, float *imag, int n)
{
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) {
            /* Swap real */
            float tr = real[i]; real[i] = real[j]; real[j] = tr;
            /* Swap imag */
            float ti = imag[i]; imag[i] = imag[j]; imag[j] = ti;
        }
    }
}

void fft_compute(float *real, float *imag, int n)
{
    /* Ensure n is power of 2 */
    if (n <= 1) return;

    /* Bit-reversal permutation */
    fft_bit_reverse(real, imag, n);

    /* Iterative Cooley-Tukey */
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * (float)M_PI / (float)len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);

        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f, w_i = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                float u_r = real[i + j];
                float u_i = imag[i + j];
                float v_r = real[i + j + len / 2] * w_r
                          - imag[i + j + len / 2] * w_i;
                float v_i = real[i + j + len / 2] * w_i
                          + imag[i + j + len / 2] * w_r;

                real[i + j] = u_r + v_r;
                imag[i + j] = u_i + v_i;
                real[i + j + len / 2] = u_r - v_r;
                imag[i + j + len / 2] = u_i - v_i;

                /* Update twiddle factor */
                float nw_r = w_r * wlen_r - w_i * wlen_i;
                float nw_i = w_r * wlen_i + w_i * wlen_r;
                w_r = nw_r;
                w_i = nw_i;
            }
        }
    }
}

/* ---- Window functions ---- */

void fft_hanning_window(float *data, int n)
{
    for (int i = 0; i < n; i++) {
        data[i] *= 0.5f * (1.0f - cosf(2.0f * (float)M_PI * (float)i / (float)(n - 1)));
    }
}

void fft_hamming_window(float *data, int n)
{
    for (int i = 0; i < n; i++) {
        data[i] *= 0.54f - 0.46f * cosf(2.0f * (float)M_PI * (float)i / (float)(n - 1));
    }
}

void fft_blackman_window(float *data, int n)
{
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)(n - 1);
        data[i] *= 0.42f - 0.5f * cosf(2.0f * (float)M_PI * t)
                    + 0.08f * cosf(4.0f * (float)M_PI * t);
    }
}

/* ---- Statistical utility functions ---- */

float compute_rms(const float *data, int n)
{
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += data[i] * data[i];
    return sqrtf(sum / (float)n);
}

float compute_mean(const float *data, int n)
{
    float sum = 0.0f;
    for (int i = 0; i < n; i++) sum += data[i];
    return sum / (float)n;
}

float compute_variance(const float *data, int n, float mean)
{
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = data[i] - mean;
        sum += d * d;
    }
    return sum / (float)n;
}

float compute_kurtosis(const float *data, int n, float mean, float variance)
{
    if (variance < 1e-8f) return 0.0f;
    float m4 = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = data[i] - mean;
        float d2 = d * d;
        m4 += d2 * d2;
    }
    m4 /= (float)n;
    return m4 / (variance * variance);
}

float compute_hjorth_activity(const float *data, int n)
{
    return compute_variance(data, n, compute_mean(data, n));
}

float compute_hjorth_mobility(const float *data, int n)
{
    if (n < 2) return 0.0f;
    float diff[FFT_SIZE];
    for (int i = 0; i < n - 1; i++) {
        diff[i] = data[i + 1] - data[i];
    }
    float var_sig = compute_variance(data, n, compute_mean(data, n));
    float var_diff = compute_variance(diff, n - 1, compute_mean(diff, n - 1));
    if (var_sig < 1e-8f) return 0.0f;
    return sqrtf(var_diff / var_sig);
}

float compute_hjorth_complexity(const float *data, int n)
{
    if (n < 3) return 0.0f;
    float mobility_sig = compute_hjorth_mobility(data, n);
    if (mobility_sig < 1e-8f) return 0.0f;

    /* Mobility of first derivative */
    float diff1[FFT_SIZE];
    for (int i = 0; i < n - 1; i++) {
        diff1[i] = data[i + 1] - data[i];
    }
    float mobility_diff1 = compute_hjorth_mobility(diff1, n - 1);

    /* Mobility of second derivative */
    float diff2[FFT_SIZE];
    for (int i = 0; i < n - 2; i++) {
        diff2[i] = data[i + 2] - 2.0f * data[i + 1] + data[i];
    }
    float mobility_diff2 = compute_hjorth_mobility(diff2, n - 2);

    if (mobility_diff1 < 1e-8f) return 0.0f;
    return mobility_diff2 / mobility_diff1;
}

float compute_spectral_centroid(const float *psd, int n_bins, float freq_res)
{
    float weighted = 0.0f, total = 0.0f;
    for (int i = 0; i < n_bins; i++) {
        weighted += (float)i * psd[i];
        total += psd[i];
    }
    if (total < 1e-8f) return 0.0f;
    return (weighted / total) * freq_res;
}

float compute_spectral_entropy(const float *psd, int n_bins, float total_power)
{
    if (total_power < 1e-8f) return 0.0f;
    float entropy = 0.0f;
    for (int i = 0; i < n_bins; i++) {
        float p = psd[i] / total_power;
        if (p > 1e-6f) {
            entropy -= p * logf(p);
        }
    }
    return entropy;
}

float compute_harmonic_ratio(const float *psd, int n_bins, int fund_bin,
                              int harmonic)
{
    int harm_bin = fund_bin * harmonic;
    if (harm_bin >= n_bins || psd[harm_bin] < 1e-6f) return 100.0f;
    return psd[fund_bin] / psd[harm_bin];
}

/* ---- Peak detection ---- */

int find_dominant_peak(const float *psd, int bin_low, int bin_high,
                       spectral_peak_t *peak)
{
    if (bin_low >= bin_high || bin_low < 0) return -1;

    float max_psd = 0.0f;
    int max_bin = bin_low;

    for (int i = bin_low; i <= bin_high; i++) {
        if (psd[i] > max_psd) {
            max_psd = psd[i];
            max_bin = i;
        }
    }

    /* Parabolic interpolation for sub-bin frequency estimation */
    float alpha = (max_bin > 0) ? psd[max_bin - 1] : 0.0f;
    float beta = psd[max_bin];
    float gamma = (max_bin < bin_high) ? psd[max_bin + 1] : 0.0f;

    float delta = 0.0f;
    float denom = alpha - 2.0f * beta + gamma;
    if (fabsf(denom) > 1e-8f) {
        delta = 0.5f * (alpha - gamma) / denom;
    }

    /* Quality factor: peak height / average of neighbors */
    float neighbor_avg = (alpha + gamma) * 0.5f;
    float qf = (neighbor_avg > 1e-6f) ? (beta / neighbor_avg) : 1.0f;

    peak->bin_index = max_bin;
    peak->frequency_hz = ((float)max_bin + delta) * FREQ_RESOLUTION_HZ;
    peak->power = beta;
    peak->quality_factor = qf;

    return 0;
}

/* ---- Watchdog feed stub (overridden by BSP in production) ---- */
__attribute__((weak)) void watchdog_feed(void) { }

/* ---- End of dsp.c ---- */