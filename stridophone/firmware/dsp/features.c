/*
 * features.c — vibration-band features + delta/delta-delta + feature packing.
 *
 * Author : jayis1
 * License: MIT
 *
 * Vibration features target substrate-borne larval feeding (termites,
 * old-house borer, furniture beetle). These produce impulsive, low-duty-cycle
 * events in the 20-500 Hz band. We compute:
 *   - spectral crest (peak/mean of |FFT|) -> high for impulsive sources
 *   - kurtosis of the time-domain RMS -> high for transient larval gnawing
 *   - pulse repetition rate -> species-specific (e.g. Reticulitermes ~5/s)
 */
#include "features.h"
#include "fft.h"
#include "../board.h"
#include <math.h>
#include <string.h>

/* Sliding history of MFCC frames for delta computation. */
#define MFCC_HIST 3
static float g_mfcc_hist[MFCC_HIST][13];
static int   g_mfcc_idx = 0;

void features_init(void) {
    memset(g_mfcc_hist, 0, sizeof(g_mfcc_hist));
    g_mfcc_idx = 0;
}

void features_vibe_spectrum(float axis[3][DSP_FRAME_N], float *mag, int n) {
    /* RMS-combine the three axes into one pseudo-signal, then FFT. */
    static float rms[DSP_FRAME_N];
    for (int i = 0; i < n; i++) {
        float x = axis[0][i], y = axis[1][i], z = axis[2][i];
        rms[i] = sqrtf(x*x + y*y + z*z);
    }
    fft_rfft_hann(rms, mag, n);
}

static float spectral_crest(const float *mag, int n) {
    int half = n/2 + 1;
    float peak = 0.0f, sum = 0.0f;
    for (int k = 1; k < half; k++) {
        if (mag[k] > peak) peak = mag[k];
        sum += mag[k];
    }
    return (sum > 1e-9f) ? peak / (sum / (half-1)) : 0.0f;
}

static float time_kurtosis(float axis[3][DSP_FRAME_N], int n) {
    /* Use the X axis as representative (or pick the noisiest). */
    float *x = axis[0];
    float mean = 0.0f;
    for (int i = 0; i < n; i++) mean += x[i];
    mean /= n;
    float m2 = 0.0f, m4 = 0.0f;
    for (int i = 0; i < n; i++) {
        float d = x[i] - mean;
        float d2 = d*d;
        m2 += d2; m4 += d2*d2;
    }
    m2 /= n; m4 /= n;
    if (m2 < 1e-12f) return 0.0f;
    return m4 / (m2 * m2);
}

static float pulse_repetition_rate(float axis[3][DSP_FRAME_N], int n) {
    /* Count threshold-crossings of the band-passed RMS envelope. */
    float *x = axis[0];
    /* Crude band-pass: differentiate (high-pass) + moving average (low-pass). */
    static float env[DSP_FRAME_N];
    float prev = x[0];
    for (int i = 1; i < n; i++) {
        float d = fabsf(x[i] - prev);
        env[i] = d;
        prev = x[i];
    }
    /* Threshold = mean + 3*std of env. */
    float mean = 0.0f;
    for (int i = 1; i < n; i++) mean += env[i];
    mean /= (n - 1);
    float var = 0.0f;
    for (int i = 1; i < n; i++) { float d = env[i] - mean; var += d*d; }
    float std = sqrtf(var / (n - 1));
    float thr = mean + 3.0f * std;
    int pulses = 0;
    int above = 0;
    for (int i = 1; i < n; i++) {
        if (env[i] > thr && !above) { pulses++; above = 1; }
        else if (env[i] < thr * 0.5f) { above = 0; }
    }
    /* Rate in Hz: pulses / (n / sample_rate). The accel is decimated to
     * DSP_FRAME_N samples over the audio hop, so effective rate ~ AUDIO_FS. */
    float dur_s = (float)n / (float)AUDIO_FS_HZ;
    return (dur_s > 1e-6f) ? (float)pulses / dur_s : 0.0f;
}

void features_pack(float *feat, const float *mfcc13,
                   const float *vibe_mag,
                   float axis[3][DSP_FRAME_N], int n,
                   float temp_c, float rh_pct) {
    /* Push MFCC into history. */
    memcpy(g_mfcc_hist[g_mfcc_idx], mfcc13, sizeof(float)*13);
    int cur = g_mfcc_idx;
    int prev1 = (g_mfcc_idx + MFCC_HIST - 1) % MFCC_HIST;
    int prev2 = (g_mfcc_idx + MFCC_HIST - 2) % MFCC_HIST;
    g_mfcc_idx = (g_mfcc_idx + 1) % MFCC_HIST;

    /* Static MFCC (13). */
    for (int i = 0; i < 13; i++) feat[i] = mfcc13[i];
    /* Delta (13): cur - prev1. */
    for (int i = 0; i < 13; i++) feat[13 + i] =
        g_mfcc_hist[cur][i] - g_mfcc_hist[prev1][i];
    /* Delta-delta (13): (cur - 2*prev1 + prev2). */
    for (int i = 0; i < 13; i++) feat[26 + i] =
        g_mfcc_hist[cur][i] - 2.0f*g_mfcc_hist[prev1][i] + g_mfcc_hist[prev2][i];

    /* Vibration features. */
    feat[39] = spectral_crest(vibe_mag, n);
    feat[40] = time_kurtosis(axis, n);
    feat[41] = pulse_repetition_rate(axis, n);
    feat[42] = temp_c;
    feat[43] = rh_pct;
}