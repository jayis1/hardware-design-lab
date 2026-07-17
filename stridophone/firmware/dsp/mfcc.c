/*
 * mfcc.c — Mel filterbank + DCT-II for MFCC extraction.
 *
 * Author : jayis1
 * License: MIT
 *
 * Standard MFCC pipeline:
 *   |FFT| -> Mel filterbank (20 triangular filters, 0-8 kHz) -> log -> DCT-II
 *
 * We keep 13 of the 20 DCT coefficients (drop the 0th sometimes; here we
 * keep it as it carries overall energy, useful for insect call detection).
 * Delta and delta-delta are computed in features.c across a sliding window.
 */
#include "mfcc.h"
#include "../board.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define N_MEL   20
#define N_DCT   13
#define F_MAX   8000.0f
#define F_MIN   0.0f

static float mel(float f)  { return 1127.0f * logf(1.0f + f / 700.0f); }
static float mel_inv(float m){ return 700.0f * (expf(m / 1127.0f) - 1.0f); }

void mfcc_init(float mel_fb[20][DSP_FRAME_N/2+1], float dct[13][20]) {
    int N = DSP_FRAME_N;
    float bin_hz = (float)AUDIO_FS_HZ / N;
    int n_bins = N/2 + 1;

    float mel_max = mel(F_MAX);
    float mel_min = mel(F_MIN);
    /* Centres of the 20 Mel filters + 2 boundary points. */
    float centres[N_MEL + 2];
    for (int i = 0; i < N_MEL + 2; i++) {
        float m = mel_min + (mel_max - mel_min) * i / (N_MEL + 1);
        centres[i] = mel_inv(m);
    }
    /* Build triangular filters. */
    for (int f = 0; f < N_MEL; f++) {
        float l = centres[f], c = centres[f+1], r = centres[f+2];
        for (int k = 0; k < n_bins; k++) {
            float freq = k * bin_hz;
            float w = 0.0f;
            if (freq >= l && freq <= c)       w = (freq - l) / (c - l + 1e-9f);
            else if (freq > c && freq <= r)  w = (r - freq) / (r - c + 1e-9f);
            mel_fb[f][k] = w;
        }
    }
    /* DCT-II matrix (orthonormal). */
    for (int n = 0; n < N_DCT; n++) {
        for (int m = 0; m < N_MEL; m++) {
            dct[n][m] = cosf((float)M_PI * n * (m + 0.5f) / N_MEL);
            if (n == 0) dct[n][m] *= 1.0f / sqrtf((float)N_MEL);
            else        dct[n][m] *= sqrtf(2.0f / N_MEL);
        }
    }
}

void mfcc_compute(const float *mag, float mel_fb[20][DSP_FRAME_N/2+1],
                  float dct[13][20], float *mfcc13) {
    int n_bins = DSP_FRAME_N/2 + 1;
    float mel_energy[N_MEL];
    for (int f = 0; f < N_MEL; f++) {
        float acc = 0.0f;
        for (int k = 0; k < n_bins; k++) acc += mel_fb[f][k] * mag[k];
        /* Log with floor to avoid log(0). */
        mel_energy[f] = logf(acc + 1e-6f);
    }
    for (int n = 0; n < N_DCT; n++) {
        float acc = 0.0f;
        for (int m = 0; m < N_MEL; m++) acc += dct[n][m] * mel_energy[m];
        mfcc13[n] = acc;
    }
}