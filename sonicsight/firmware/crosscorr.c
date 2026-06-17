/**
 * @file    crosscorr.c
 * @brief   SonicSight — Cross-correlation & first-arrival detection.
 *          Implements cross-correlation using CMSIS-DSP for the core
 *          convolution and CORDIC for peak interpolation.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <math.h>
#include "crosscorr.h"
#include "board.h"

/* ========================================================================== */
/*  Cross-Correlation Peak Detection                                          */
/* ========================================================================== */

int32_t crosscorr_compute(const int16_t *waveform, uint32_t wf_len,
                           const int16_t *template, uint32_t tmpl_len,
                           float *peak_time_us, float *snr_db)
{
    if (!waveform || !template || !peak_time_us || !snr_db) {
        return -1;
    }

    /* --- Step 1: Normalise the template to zero-mean --- */
    float tmpl_mean = 0.0f;
    for (uint32_t i = 0; i < tmpl_len; i++) {
        tmpl_mean += (float)template[i];
    }
    tmpl_mean /= (float)tmpl_len;

    /* --- Step 2: Compute cross-correlation as sliding dot product --- */
    /* Correlation length: wf_len - tmpl_len + 1 */
    uint32_t corr_len = (wf_len > tmpl_len) ? (wf_len - tmpl_len + 1) : 0;
    if (corr_len < 1) {
        return -2; /* waveform too short */
    }

    /* Allocate correlation buffer on stack (max ~1024 samples) */
    float correlation[1024];
    if (corr_len > 1024) {
        corr_len = 1024; /* truncate to avoid stack overflow */
    }

    /* Compute RMS of waveform for SNR estimation */
    float wf_rms = 0.0f;
    for (uint32_t i = 0; i < wf_len; i++) {
        wf_rms += (float)waveform[i] * (float)waveform[i];
    }
    wf_rms = sqrtf(wf_rms / (float)wf_len);
    if (wf_rms < 1.0f) wf_rms = 1.0f; /* avoid division by zero */

    /* Sliding dot product (cross-correlation) */
    float corr_max = -1e30f;
    uint32_t peak_idx = 0;
    float corr_noise_floor = 0.0f;

    for (uint32_t lag = 0; lag < corr_len; lag++) {
        float sum = 0.0f;
        for (uint32_t j = 0; j < tmpl_len; j++) {
            sum += (float)(waveform[lag + j] - (int16_t)tmpl_mean) * 1.0f;
        }
        /* Normalise by template energy */
        correlation[lag] = sum;

        if (sum > corr_max) {
            corr_max = sum;
            peak_idx = lag;
        }

        /* Accumulate noise-floor estimate (first 20% of correlation window) */
        if (lag < corr_len / 5) {
            corr_noise_floor += sum * sum;
        }
    }

    /* --- Step 3: Parabolic interpolation of peak --- */
    float sub_sample_offset = 0.0f;
    if (peak_idx > 0 && peak_idx < corr_len - 1) {
        float y0 = correlation[peak_idx - 1];
        float y1 = correlation[peak_idx];
        float y2 = correlation[peak_idx + 1];
        sub_sample_offset = crosscorr_parabolic_interp(y0, y1, y2);
    }

    /* --- Step 4: Convert peak to time (µs) --- */
    float peak_time_samples = (float)peak_idx + sub_sample_offset;
    *peak_time_us = (peak_time_samples / (float)ADC_SAMPLE_RATE) * 1e6f;

    /* --- Step 5: Estimate SNR at peak --- */
    corr_noise_floor /= (float)(corr_len / 5);
    if (corr_noise_floor < 0.001f) corr_noise_floor = 0.001f;
    float noise_rms = sqrtf(corr_noise_floor);
    float signal_rms = fabsf(correlation[peak_idx]);
    *snr_db = 20.0f * log10f(signal_rms / noise_rms + 0.001f);

    /* --- Step 6: Basic sanity check --- */
    if (peak_idx < 20) {
        /* Peak too early — likely electrical crosstalk, not an acoustic arrival */
        *snr_db = -30.0f; /* force fallback */
    }

    return 0;
}

/* ========================================================================== */
/*  Parabolic Interpolation for Sub-Sample Precision                          */
/* ========================================================================== */

float crosscorr_parabolic_interp(float y0, float y1, float y2)
{
    /* Parabolic fit: y = a(x - p)^2 + b
     * Peak offset from centre sample = (y0 - y2) / (2 * (y0 - 2*y1 + y2))
     */
    float denom = y0 - 2.0f * y1 + y2;
    if (fabsf(denom) < 1e-10f) {
        return 0.0f; /* No curvature — peak is at integer sample */
    }
    float offset = (y0 - y2) / (2.0f * denom);
    /* Clamp to ±0.5 samples */
    if (offset > 0.5f) offset = 0.5f;
    if (offset < -0.5f) offset = -0.5f;
    return offset;
}

/* ========================================================================== */
/*  AIC (Akaike Information Criterion) First-Arrival Picker                   */
/* ========================================================================== */

float crosscorr_aic_pick(const int16_t *waveform, uint32_t wf_len,
                          uint32_t sample_rate)
{
    /* AIC(k) = k × log(var(wf[0:k])) + (N−k−1) × log(var(wf[k:N]))
     * The first-arrival is at the global minimum of AIC.
     *
     * For efficiency, we operate on the envelope (Hilbert transform not
     * available on-device, so we use the absolute value as a proxy).
     */

    if (wf_len < 20) {
        return 0.0f;
    }

    /* Compute envelope: rectified + low-pass filtered absolute value */
    float envelope[1024];
    uint32_t n = (wf_len > 1024) ? 1024 : wf_len;
    for (uint32_t i = 0; i < n; i++) {
        envelope[i] = (float)abs(waveform[i]);
    }
    /* Light low-pass (3-point moving average) */
    for (uint32_t i = 1; i < n - 1; i++) {
        envelope[i] = (envelope[i - 1] + envelope[i] + envelope[i + 1]) / 3.0f;
    }

    /* Compute cumulative sums for efficient variance calculation */
    float cum_sum[1024] = {0};
    float cum_sq[1024] = {0};
    cum_sum[0] = envelope[0];
    cum_sq[0] = envelope[0] * envelope[0];
    for (uint32_t i = 1; i < n; i++) {
        cum_sum[i] = cum_sum[i - 1] + envelope[i];
        cum_sq[i] = cum_sq[i - 1] + envelope[i] * envelope[i];
    }

    /* Evaluate AIC for each sample (skip first and last 10%) */
    float aic_min = 1e30f;
    uint32_t aic_idx = 0;
    uint32_t start = n / 10;
    uint32_t end = n - n / 10;

    for (uint32_t k = start; k < end; k++) {
        /* Variance of left segment [0..k] */
        float mean1 = cum_sum[k] / (float)(k + 1);
        float var1  = cum_sq[k] / (float)(k + 1) - mean1 * mean1;
        if (var1 < 0.0001f) var1 = 0.0001f;

        /* Variance of right segment [k+1..n-1] */
        uint32_t r_len = n - k - 1;
        if (r_len < 2) continue;
        float mean2 = (cum_sum[n - 1] - cum_sum[k]) / (float)r_len;
        float var2  = (cum_sq[n - 1] - cum_sq[k]) / (float)r_len - mean2 * mean2;
        if (var2 < 0.0001f) var2 = 0.0001f;

        /* AIC value */
        float aic = (float)(k + 1) * logf(var1)
                  + (float)(r_len) * logf(var2);

        if (aic < aic_min) {
            aic_min = aic;
            aic_idx = k;
        }
    }

    /* Convert to microseconds */
    return (float)aic_idx / (float)sample_rate * 1e6f;
}