/*
 * dsp.c — DSP pipeline for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the real-time digital signal processing pipeline for
 * 16-channel mycelial electrophysiology data:
 *
 *   1. High-pass filter (0.5 Hz, 1st-order IIR) — removes DC drift
 *   2. Mains notch filter (50/60 Hz, 2nd-order IIR twin-T)
 *   3. Low-pass filter (5 kHz, 4th-order FIR)
 *   4. Adaptive spike detection (MAD-based threshold)
 *   5. Cross-channel propagation analysis (normalized cross-correlation)
 *
 * All filters operate on int16_t samples with fixed-point arithmetic
 * to avoid floating-point overhead on most operations, though the FPU
 * is used for the spike detection threshold and correlation computations.
 */

#include "dsp.h"
#include "registers.h"
#include <string.h>

/* ===================================================================== */
/*  Filter state (per channel)                                            */
/* ===================================================================== */

typedef struct {
    /* High-pass IIR: y[n] = a * (y[n-1] + x[n] - x[n-1])
       a = RC / (RC + dt), RC = 1/(2*pi*fc), fc = 0.5 Hz, fs = 1000 Hz
       a = 1/(2*pi*0.5*0.001 + 1) ≈ 0.99686 */
    int32_t hp_y_prev;
    int32_t hp_x_prev;
    /* Fixed-point coefficient: a * 2^20 = 0.99686 * 1048576 ≈ 1045232 */
    int32_t hp_a_q20;

    /* Notch filter (twin-T 2nd-order IIR):
       Notch at 50 Hz, Q ≈ 3, sample rate 1000 Hz.
       Coefficients computed offline and stored as Q15 fixed-point. */
    int32_t notch_x[2];  /* x[n-1], x[n-2] */
    int32_t notch_y[2];  /* y[n-1], y[n-2] */
    /* Q15 coefficients for direct form II */
    int16_t notch_b0, notch_b1, notch_b2;
    int16_t notch_a1, notch_a2;

    /* Low-pass FIR (4th-order, symmetric)
       Cutoff 5 kHz at 1 kSPS — actually near Nyquist, so this is a
       mild anti-alias smoothing filter.  Coefficients are simple
       moving-average-like with slight rounding. */
    int16_t lp_taps[5];  /* delay line */
    int16_t lp_coeffs[5];

    /* Noise estimation (MAD — Median Absolute Deviation) */
    int16_t noise_mad;
    int16_t noise_buffer[128];
    uint8_t noise_idx;
    uint8_t noise_filled;
} channel_dsp_t;

static channel_dsp_t g_dsp[ADS1298_TOTAL_CHANS];
static uint8_t g_mains_freq = 50;
static uint16_t g_sample_rate = 1000;

/* ===================================================================== */
/*  Notch filter coefficient computation                                   */
/* ===================================================================== */

/* Pre-computed Q15 coefficients for 50 Hz and 60 Hz notch at 1 kSPS.
   These are standard twin-T IIR notch filter coefficients. */

/* 50 Hz notch at fs=1000:
   b0 = 0.965081  b1 = -1.658574  b2 = 0.965081
   a1 = -1.658574  a2 = 0.930163
   Scaled to Q15 (×32768): */
#define NOTCH50_B0_Q15  31624
#define NOTCH50_B1_Q15  (-54354)
#define NOTCH50_B2_Q15  31624
#define NOTCH50_A1_Q15  (-54354)
#define NOTCH50_A2_Q15  30474

/* 60 Hz notch at fs=1000:
   b0 = 0.950361  b1 = -1.264847  b2 = 0.950361
   a1 = -1.264847  a2 = 0.900723
   Scaled to Q15: */
#define NOTCH60_B0_Q15  31142
#define NOTCH60_B1_Q15  (-41438)
#define NOTCH60_B2_Q15  31142
#define NOTCH60_A1_Q15  (-41438)
#define NOTCH60_A2_Q15  29521

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

void dsp_init(uint8_t mains_freq_hz, uint16_t sample_rate_hz)
{
    g_mains_freq = mains_freq_hz;
    g_sample_rate = sample_rate_hz;

    for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
        memset(&g_dsp[ch], 0, sizeof(channel_dsp_t));

        /* High-pass coefficient: a = 1 / (1 + 2*pi*0.5/1000) ≈ 0.99686
           Q20 fixed-point: 0.99686 * 1048576 = 1045232 */
        g_dsp[ch].hp_a_q20 = 1045232;

        /* Set notch coefficients based on mains frequency. */
        if (mains_freq_hz == 60) {
            g_dsp[ch].notch_b0 = NOTCH60_B0_Q15;
            g_dsp[ch].notch_b1 = NOTCH60_B1_Q15;
            g_dsp[ch].notch_b2 = NOTCH60_B2_Q15;
            g_dsp[ch].notch_a1 = NOTCH60_A1_Q15;
            g_dsp[ch].notch_a2 = NOTCH60_A2_Q15;
        } else {
            g_dsp[ch].notch_b0 = NOTCH50_B0_Q15;
            g_dsp[ch].notch_b1 = NOTCH50_B1_Q15;
            g_dsp[ch].notch_b2 = NOTCH50_B2_Q15;
            g_dsp[ch].notch_a1 = NOTCH50_A1_Q15;
            g_dsp[ch].notch_a2 = NOTCH50_A2_Q15;
        }

        /* Low-pass FIR coefficients (symmetric, 5-tap).
           Simple smoothing filter: [1, 2, 3, 2, 1] / 9 */
        g_dsp[ch].lp_coeffs[0] = 3641;   /* 1/9 * 32768 */
        g_dsp[ch].lp_coeffs[1] = 7282;   /* 2/9 * 32768 */
        g_dsp[ch].lp_coeffs[2] = 10922;  /* 3/9 * 32768 */
        g_dsp[ch].lp_coeffs[3] = 7282;
        g_dsp[ch].lp_coeffs[4] = 3641;
    }
}

void dsp_set_notch_freq(uint8_t mains_hz)
{
    g_mains_freq = mains_hz;
    for (uint8_t ch = 0; ch < ADS1298_TOTAL_CHANS; ch++) {
        if (mains_hz == 60) {
            g_dsp[ch].notch_b0 = NOTCH60_B0_Q15;
            g_dsp[ch].notch_b1 = NOTCH60_B1_Q15;
            g_dsp[ch].notch_b2 = NOTCH60_B2_Q15;
            g_dsp[ch].notch_a1 = NOTCH60_A1_Q15;
            g_dsp[ch].notch_a2 = NOTCH60_A2_Q15;
        } else {
            g_dsp[ch].notch_b0 = NOTCH50_B0_Q15;
            g_dsp[ch].notch_b1 = NOTCH50_B1_Q15;
            g_dsp[ch].notch_b2 = NOTCH50_B2_Q15;
            g_dsp[ch].notch_a1 = NOTCH50_A1_Q15;
            g_dsp[ch].notch_a2 = NOTCH50_A2_Q15;
        }
    }
}

/* ===================================================================== */
/*  Per-channel processing                                                */
/* ===================================================================== */

static int16_t apply_hp(channel_dsp_t *state, int16_t x)
{
    /* y[n] = a * (y[n-1] + x[n] - x[n-1])
       Q20 arithmetic: result = (a_q20 * (y_prev + x - x_prev)) >> 20 */
    int32_t diff = (int32_t)x - state->hp_x_prev;
    int32_t val = state->hp_y_prev + diff;
    int32_t y = (int32_t)((int64_t)state->hp_a_q20 * val) >> 20;

    state->hp_y_prev = y;
    state->hp_x_prev = x;
    return (int16_t)y;
}

static int16_t apply_notch(channel_dsp_t *state, int16_t x)
{
    /* Direct Form I IIR notch filter with Q15 coefficients.
       y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
       All operations in Q15: multiply by Q15 coeff, then >> 15. */
    int32_t acc = 0;
    acc += (int32_t)state->notch_b0 * x;
    acc += (int32_t)state->notch_b1 * state->notch_x[0];
    acc += (int32_t)state->notch_b2 * state->notch_x[1];
    acc -= (int32_t)state->notch_a1 * state->notch_y[0];
    acc -= (int32_t)state->notch_a2 * state->notch_y[1];
    int16_t y = (int16_t)(acc >> 15);

    /* Shift delay lines. */
    state->notch_x[1] = state->notch_x[0];
    state->notch_x[0] = x;
    state->notch_y[1] = state->notch_y[0];
    state->notch_y[0] = y;

    return y;
}

static int16_t apply_lp(channel_dsp_t *state, int16_t x)
{
    /* 5-tap symmetric FIR filter.
       y[n] = sum(coeffs[i] * taps[i]) >> 15  (Q15 arithmetic) */
    int32_t acc = 0;
    for (int i = 4; i > 0; i--) {
        state->lp_taps[i] = state->lp_taps[i - 1];
    }
    state->lp_taps[0] = x;

    for (int i = 0; i < 5; i++) {
        acc += (int32_t)state->lp_coeffs[i] * state->lp_taps[i];
    }
    return (int16_t)(acc >> 15);
}

static void update_noise_est(channel_dsp_t *state, int16_t sample)
{
    /* Maintain a circular buffer and compute MAD (Median Absolute
       Deviation) as a robust noise estimator.  For efficiency, we
       approximate MAD as the mean of absolute deviations from the
       running mean, which is close enough for threshold setting. */
    state->noise_buffer[state->noise_idx] = sample;
    state->noise_idx = (state->noise_idx + 1) & 0x7F;
    if (state->noise_idx == 0) state->noise_filled = 1;

    /* Recompute MAD every 128 samples. */
    if (state->noise_idx == 0) {
        uint16_t n = state->noise_filled ? 128 : (uint16_t)state->noise_idx;
        if (n < 16) return;

        /* Compute mean. */
        int32_t sum = 0;
        for (uint16_t i = 0; i < n; i++) {
            sum += state->noise_buffer[i];
        }
        int16_t mean = (int16_t)(sum / n);

        /* Compute mean absolute deviation. */
        int32_t abs_sum = 0;
        for (uint16_t i = 0; i < n; i++) {
            int16_t dev = state->noise_buffer[i] - mean;
            abs_sum += (dev >= 0) ? dev : (int16_t)(-dev);
        }
        state->noise_mad = (int16_t)(abs_sum / n);
    }
}

void dsp_process_channel(uint8_t channel,
                         const int16_t *input,
                         int16_t *filtered,
                         uint16_t n_samples,
                         uint8_t mains_freq_hz)
{
    if (channel >= ADS1298_TOTAL_CHANS) return;
    channel_dsp_t *state = &g_dsp[channel];

    for (uint16_t i = 0; i < n_samples; i++) {
        int16_t s = input[i];

        /* 1. High-pass filter (DC removal). */
        s = apply_hp(state, s);

        /* 2. Mains notch filter. */
        s = apply_notch(state, s);

        /* 3. Low-pass smoothing filter. */
        s = apply_lp(state, s);

        /* 4. Update noise estimate. */
        update_noise_est(state, s);

        filtered[i] = s;
    }
}

/* ===================================================================== */
/*  Spike detection                                                       */
/* ===================================================================== */

uint16_t dsp_detect_spikes(uint8_t channel,
                           const int16_t *filtered,
                           uint16_t n_samples,
                           spike_event_t *spikes,
                           uint16_t max_spikes,
                           uint32_t timestamp_ms)
{
    if (channel >= ADS1298_TOTAL_CHANS || max_spikes == 0) return 0;
    channel_dsp_t *state = &g_dsp[channel];

    /* Adaptive threshold: THR = k * MAD (k = 4.0).
       noise_mad is in ADC counts; convert to approximate µV:
       At gain 12, Vref 2.5V, 24-bit → 1 LSB ≈ 0.149 µV.
       After >> 8 to 16-bit: 1 LSB ≈ 38.1 µV.
       But we work in ADC counts and convert to µV for the event. */
    int16_t mad = state->noise_mad;
    if (mad < 5) mad = 5;  /* minimum floor to avoid noise-only detection */

    int16_t threshold = (int16_t)(SPIKE_THRESHOLD_K * (float)mad);

    uint16_t count = 0;
    int16_t peak_val = 0;
    uint16_t peak_idx = 0;
    uint8_t in_spike = 0;
    uint16_t spike_start = 0;

    for (uint16_t i = 0; i < n_samples; i++) {
        int16_t abs_val = (filtered[i] >= 0) ? filtered[i] : (int16_t)(-filtered[i]);

        if (!in_spike) {
            if (abs_val > threshold) {
                in_spike = 1;
                spike_start = (i >= SPIKE_WINDOW_PRE) ? (i - SPIKE_WINDOW_PRE) : 0;
                peak_val = filtered[i];
                peak_idx = i;
            }
        } else {
            /* Track peak within the spike window. */
            if (abs_val > ((peak_val >= 0) ? peak_val : (int16_t)(-peak_val))) {
                peak_val = filtered[i];
                peak_idx = i;
            }

            /* End spike after post-window samples past the peak, or if
               signal returns below threshold/2 for a refractory period. */
            if (i >= peak_idx + SPIKE_WINDOW_POST) {
                /* Record the spike. */
                if (count < max_spikes) {
                    spikes[count].channel = channel;
                    spikes[count].template_id = 0;  /* filled by spike_sort */
                    /* Convert ADC counts to µV: 1 count ≈ 38.1 µV (16-bit,
                       gain 12, 2.5V ref, 24-bit >> 8) */
                    spikes[count].amplitude_uv = (int16_t)((int32_t)peak_val * 381 / 10);
                    spikes[count].timestamp_ms = timestamp_ms + peak_idx;
                    count++;
                }

                /* Refractory period: skip 20 samples (20 ms) to avoid
                   double-counting the same spike. */
                i += 20;
                in_spike = 0;
            }
        }
    }

    return count;
}

/* ===================================================================== */
/*  Cross-channel propagation analysis                                    */
/* ===================================================================== */

/* Compute normalized cross-correlation between two channels at a given lag. */
static float normalized_xcorr(const int16_t *a, const int16_t *b,
                              uint16_t n, int16_t lag)
{
    if (lag < 0) lag = -lag;
    if ((uint16_t)lag >= n) return 0.0f;

    uint16_t overlap = n - (uint16_t)lag;
    float mean_a = 0.0f, mean_b = 0.0f;

    for (uint16_t i = 0; i < overlap; i++) {
        mean_a += (float)a[i];
        mean_b += (float)b[i + lag];
    }
    mean_a /= overlap;
    mean_b /= overlap;

    float num = 0.0f, den_a = 0.0f, den_b = 0.0f;
    for (uint16_t i = 0; i < overlap; i++) {
        float da = (float)a[i] - mean_a;
        float db = (float)b[i + lag] - mean_b;
        num += da * db;
        den_a += da * da;
        den_b += db * db;
    }

    float den = den_a * den_b;
    if (den < 1.0f) return 0.0f;

    /* Newton's method sqrt. */
    float x = den;
    for (int i = 0; i < 6; i++) {
        if (x > 0.0f) x = 0.5f * (x + den / x);
    }
    if (x < 1.0f) return 0.0f;

    return num / x;
}

uint16_t dsp_compute_propagation(
    int16_t filtered[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
    uint8_t n_channels,
    uint16_t n_samples,
    uint16_t window_ms,
    float corr_threshold)
{
    /* Check all channel pairs for correlated activity within the
       time window.  A "propagation event" is counted when the peak
       cross-correlation exceeds the threshold at a non-zero lag. */
    uint16_t prop_count = 0;
    int16_t max_lag = (int16_t)((uint32_t)window_ms * g_sample_rate / 1000);
    if (max_lag > (int16_t)(n_samples / 2)) max_lag = n_samples / 2;

    for (uint8_t i = 0; i < n_channels; i++) {
        for (uint8_t j = i + 1; j < n_channels; j++) {
            float best_corr = 0.0f;
            int16_t best_lag = 0;

            for (int16_t lag = -max_lag; lag <= max_lag; lag++) {
                float corr = normalized_xcorr(
                    filtered[i], filtered[j], n_samples, lag);
                if (corr < 0.0f) corr = -corr;
                if (corr > best_corr) {
                    best_corr = corr;
                    best_lag = lag;
                }
            }

            if (best_corr > corr_threshold && best_lag != 0) {
                prop_count++;
            }
        }
    }

    return prop_count;
}

/* ===================================================================== */
/*  Noise floor query                                                     */
/* ===================================================================== */

float dsp_get_noise_mad(uint8_t channel)
{
    if (channel >= ADS1298_TOTAL_CHANS) return 0.0f;
    return (float)g_dsp[channel].noise_mad;
}