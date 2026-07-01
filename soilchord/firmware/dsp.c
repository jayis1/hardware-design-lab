/*
 * dsp.c — Soilchord DSP: FFT, peak detection, Q estimation, anomaly
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Uses CMSIS-DSP arm_rfft_fast_f32 for the 4096-point real FFT. We vendor a
 * minimal stub of the CMSIS-DSP API in this reference so the code compiles
 * standalone; in a real build link against the CMSIS-DSP library.
 */
#include <math.h>
#include <string.h>
#include "soilchord.h"
#include "board.h"

/* ---- CMSIS-DSP API surface (vendored minimal implementation) ------------ */
/* We implement a simple radix-2 DFT here so the reference is self-contained.
 * A production build links arm_rfft_fast_f32 from CMSIS-DSP. */
#define PI32  (3.14159265358979323846f)

/* Hann window cache, computed once. */
static float s_hann[FFT_SIZE];
static float s_fft_in[FFT_SIZE];
static float s_fft_out[FFT_SIZE];     /* magnitudes (half spectrum) */
static float s_rfft_complex[FFT_SIZE]; /* real+imag interleaved for our DFT */

void dsp_init(void)
{
    for (int n = 0; n < FFT_SIZE; n++) {
        s_hann[n] = 0.5f * (1.0f - cosf(2.0f * PI32 * (float)n / (float)(FFT_SIZE - 1)));
    }
}

/* Naive DFT (O(N^2)); used only so this reference compiles without CMSIS.
 * Replace with arm_rfft_fast_f32 in the production build. */
static void naive_real_dft(const float *in, float *out_mag, int n)
{
    int half = n / 2;
    for (int k = 0; k <= half; k++) {
        float re = 0.0f, im = 0.0f;
        float wk = 2.0f * PI32 * (float)k / (float)n;
        /* This is intentionally simple; for 4096 points it is slow but the
         * reference firmware is buildable and the structure is what matters. */
        for (int t = 0; t < n; t++) {
            float w = wk * (float)t;
            re += in[t] * cosf(w);
            im -= in[t] * sinf(w);
        }
        out_mag[k] = sqrtf(re * re + im * im) * (2.0f / (float)n);
    }
}

/* ---- Time-domain helpers ---------------------------------------------- */
static float compute_rms(const float *x, int n)
{
    float s = 0.0f;
    for (int i = 0; i < n; i++) s += x[i] * x[i];
    return sqrtf(s / (float)n);
}

static float compute_crest(const float *x, int n, float rms)
{
    float pk = 0.0f;
    for (int i = 0; i < n; i++) {
        float v = fabsf(x[i]);
        if (v > pk) pk = v;
    }
    return (rms > 1e-9f) ? pk / rms : 0.0f;
}

/* Fit exponential decay to the RMS envelope (10 ms windows) → τ.
 * Q_time = π · f0 · τ */
static float ringdown_tau(const float *x, int n, float fs)
{
    int win = (int)(fs * 0.010f);   /* 10 ms */
    if (win < 1) win = 1;
    int nseg = n / win;
    if (nseg < 4) return 0.0f;

    /* Compute per-window RMS → y[k]; fit ln(y) = a - t/τ. */
    float sum_t = 0, sum_tt = 0, sum_y = 0, sum_ty = 0;
    int cnt = 0;
    for (int k = 0; k < nseg; k++) {
        float s = 0.0f;
        for (int i = 0; i < win; i++) s += x[k * win + i] * x[k * win + i];
        float rms = sqrtf(s / (float)win);
        if (rms < 1e-3f) continue;       /* skip silent tail */
        float t  = (float)k * (float)win / fs;
        float ly = logf(rms);
        sum_t  += t;  sum_tt += t * t;
        sum_y  += ly; sum_ty += t * ly;
        cnt++;
    }
    if (cnt < 4) return 0.0f;
    float den = (float)cnt * sum_tt - sum_t * sum_t;
    if (fabsf(den) < 1e-12f) return 0.0f;
    /* slope = (cnt*sum_ty - sum_t*sum_y) / den ; τ = -1/slope */
    float slope = ((float)cnt * sum_ty - sum_t * sum_y) / den;
    if (slope >= -1e-6f) return 0.0f;    /* not decaying */
    return -1.0f / slope;
}

/* ---- Frequency-domain helpers ----------------------------------------- */
static int find_peak_in_band(const float *mag, int half, float f0_nom, float fs)
{
    int k_low  = (int)((f0_nom * (1.0f - PEAK_BAND_FRAC)) * FFT_SIZE / fs);
    int k_high = (int)((f0_nom * (1.0f + PEAK_BAND_FRAC)) * FFT_SIZE / fs);
    if (k_low < 1) k_low = 1;
    if (k_high >= half) k_high = half - 1;

    int k_peak = k_low;
    float v_peak = mag[k_low];
    for (int k = k_low + 1; k <= k_high; k++) {
        if (mag[k] > v_peak) { v_peak = mag[k]; k_peak = k; }
    }

    /* Parabolic interpolation for sub-bin accuracy. */
    if (k_peak > 0 && k_peak < half) {
        float ym = mag[k_peak - 1];
        float y0 = mag[k_peak];
        float yp = mag[k_peak + 1];
        float denom = (ym - 2.0f * y0 + yp);
        if (fabsf(denom) > 1e-9f) {
            float delta = 0.5f * (ym - yp) / denom;
            if (fabsf(delta) < 1.0f) {
                return k_peak * 1000 + (int)(delta * 1000.0f);  /* encode fractional bin */
            }
        }
    }
    return k_peak * 1000;
}

static float peak_freq_hz(int encoded, float fs)
{
    float bin = (float)(encoded / 1000) + (float)(encoded % 1000) / 1000.0f;
    return bin * fs / (float)FFT_SIZE;
}

/* −3 dB bandwidth around the peak (in Hz). */
static float minus3db_bw(const float *mag, int half, int k_peak, float fs)
{
    float pk = mag[k_peak];
    float half_power = pk * 0.70710678f;     /* -3 dB (amplitude) */
    int kl = k_peak, kh = k_peak;
    while (kl > 0 && mag[kl] > half_power) kl--;
    while (kh < half && mag[kh] > half_power) kh++;
    float bw = ((float)kh - (float)kl) * fs / (float)FFT_SIZE;
    return bw;
}

/* Harmonic power ratio: power near (n·f0) / power near f0. */
static float harmonic_ratio(const float *mag, int half, float f0_hz, int n_h, float fs)
{
    int k_f0 = (int)(f0_hz * FFT_SIZE / fs);
    int k_h  = (int)((n_h * f0_hz) * FFT_SIZE / fs);
    if (k_h >= half) return 0.0f;
    /* Sum ±2 bins around each. */
    auto_sum:
    {
        float pf = 0.0f, ph = 0.0f;
        for (int dk = -2; dk <= 2; dk++) {
            int kk = k_f0 + dk; if (kk >= 0 && kk <= half) pf += mag[kk] * mag[kk];
            int kh2 = k_h + dk; if (kh2 >= 0 && kh2 <= half) ph += mag[kh2] * mag[kh2];
        }
        return (pf > 1e-12f) ? ph / pf : 0.0f;
    }
}

/* ---- Public entry ----------------------------------------------------- */
void dsp_extract_features(const int16_t *samples, size_t n, uint8_t chord,
                          chord_features_t *out)
{
    /* 1. Copy, window, and convert to float. */
    int N = (int)n;
    if (N > FFT_SIZE) N = FFT_SIZE;
    for (int i = 0; i < N; i++) {
        s_fft_in[i] = (float)samples[i] * s_hann[i];
    }
    for (int i = N; i < FFT_SIZE; i++) s_fft_in[i] = 0.0f;

    /* 2. RMS / crest in time domain. */
    float rms = compute_rms(s_fft_in, FFT_SIZE);
    out->rms = rms;
    out->crest = compute_crest(s_fft_in, FFT_SIZE, rms);

    /* 3. Spectrum. */
    naive_real_dft(s_fft_in, s_fft_out, FFT_SIZE);
    int half = FFT_SIZE / 2;

    /* 4. Fundamental frequency. */
    float f0_nom = kChordFreeAirHz[chord];
    int k_enc = find_peak_in_band(s_fft_out, half, f0_nom, (float)SAMPLE_RATE_HZ);
    float f0 = peak_freq_hz(k_enc, (float)SAMPLE_RATE_HZ);
    out->f0 = f0;

    int k_peak = k_enc / 1000;
    if (k_peak < 1) k_peak = 1;
    if (k_peak > half - 1) k_peak = half - 1;

    /* 5. Q from -3 dB bandwidth. */
    float bw = minus3db_bw(s_fft_out, half, k_peak, (float)SAMPLE_RATE_HZ);
    out->q_freq = (bw > 1e-3f) ? (f0 / bw) : 0.0f;

    /* 6. Q from ring-down. */
    /* Re-window without the Hann for the time-domain decay fit (use the raw
     * waveform; Hann suppresses the early-time energy we want to see). */
    for (int i = 0; i < N; i++) s_fft_in[i] = (float)samples[i];
    for (int i = N; i < FFT_SIZE; i++) s_fft_in[i] = 0.0f;
    float tau = ringdown_tau(s_fft_in, FFT_SIZE, (float)SAMPLE_RATE_HZ);
    out->q_time = (tau > 0.0f) ? (PI32 * f0 * tau) : 0.0f;

    /* 7. Harmonic ratios. */
    out->h2_h1 = harmonic_ratio(s_fft_out, half, f0, 2, (float)SAMPLE_RATE_HZ);
    out->h3_h1 = harmonic_ratio(s_fft_out, half, f0, 3, (float)SAMPLE_RATE_HZ);

    /* 8. Temperature already filled in by caller. */
}