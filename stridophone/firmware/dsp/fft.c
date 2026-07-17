/*
 * fft.c — 1024-point real FFT with Hann window.
 *
 * Author : jayis1
 * License: MIT
 *
 * Rather than pull in the full CMSIS-DSP library (which is large and would
 * dominate this repo), we implement a compact radix-2 Cooley-Tukey real FFT.
 * The implementation is not the fastest possible but is self-contained,
 * deterministic, and easy to audit. On a 480 MHz Cortex-M7F a 1024-point
 * real FFT takes roughly 0.5 ms here vs 0.11 ms for CMSIS — acceptable for
 * a 1 Hz classifier and a ~21 ms hop rate.
 *
 * Pipeline: copy real input -> interleaved complex, apply Hann, run
 * forward CFFT, then pack the half-spectrum into magnitudes.
 */
#include "fft.h"
#include "../board.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float g_hann[DSP_FRAME_N];
static float g_cfft[DSP_FRAME_N * 2];   /* interleaved complex scratch */

static void cfft_butterfly(float *data, int n, int inverse);
static void bit_reverse(float *data, int n);

void fft_init(void) {
    /* Precompute the Hann window. */
    for (int i = 0; i < DSP_FRAME_N; i++) {
        g_hann[i] = 0.5f * (1.0f - cosf(2.0f * (float)M_PI * i / (DSP_FRAME_N - 1)));
    }
}

void fft_rfft(const float *pcm, float *out, int n) {
    /* Copy real input into interleaved complex buffer, zero imag. */
    for (int i = 0; i < n; i++) {
        g_cfft[2*i]   = pcm[i];
        g_cfft[2*i+1] = 0.0f;
    }
    cfft_butterfly(g_cfft, n, 0);
    /* Output interleaved complex (length n). */
    for (int i = 0; i < n; i++) {
        out[2*i]   = g_cfft[2*i];
        out[2*i+1] = g_cfft[2*i+1];
    }
}

void fft_rfft_hann(const float *pcm, float *mag, int n) {
    /* Window + copy into complex scratch. */
    for (int i = 0; i < n; i++) {
        g_cfft[2*i]   = pcm[i] * g_hann[i];
        g_cfft[2*i+1] = 0.0f;
    }
    cfft_butterfly(g_cfft, n, 0);
    /* Magnitude of first n/2+1 bins. */
    int half = n / 2;
    for (int k = 0; k <= half; k++) {
        float re = g_cfft[2*k];
        float im = g_cfft[2*k+1];
        mag[k] = sqrtf(re*re + im*im);
    }
}

/* ---- In-place radix-2 CFFT --------------------------------------- */
static void bit_reverse(float *data, int n) {
    int j = 0;
    for (int i = 1; i < 2*n - 1; i += 2) {
        int m = n;
        do { m >>= 1; j ^= m; } while ((j & m) == 0 && m > 1);
        if (j > i) {
            float tr = data[i];   data[i]   = data[j];   data[j]   = tr;
            float ti = data[i+1]; data[i+1] = data[j+1]; data[j+1] = ti;
        }
    }
}

static void cfft_butterfly(float *data, int n, int inverse) {
    bit_reverse(data, n);
    float sign = inverse ? -1.0f : 1.0f;
    for (int s = 1; s <= (int)log2f((float)n); s++) {
        int m  = 1 << s;
        int m2 = m >> 1;
        float wm_re = cosf(sign * (float)M_PI / m2);
        float wm_im = sinh(0.0f);  /* placeholder; use sin below */
        wm_im = sinf(sign * (float)M_PI / m2);
        for (int k = 0; k < n; k += m) {
            float w_re = 1.0f, w_im = 0.0f;
            for (int j = 0; j < m2; j++) {
                int idx1 = 2*(k+j);
                int idx2 = 2*(k+j+m2);
                float t_re = w_re * data[idx2] - w_im * data[idx2+1];
                float t_im = w_re * data[idx2+1] + w_im * data[idx2];
                data[idx2]   = data[idx1]   - t_re;
                data[idx2+1] = data[idx1+1] - t_im;
                data[idx1]   = data[idx1]   + t_re;
                data[idx1+1] = data[idx1+1] + t_im;
                /* w *= wm */
                float nw_re = w_re * wm_re - w_im * wm_im;
                float nw_im = w_re * wm_im + w_im * wm_re;
                w_re = nw_re; w_im = nw_im;
            }
        }
    }
    if (inverse) {
        for (int i = 0; i < 2*n; i++) data[i] /= (float)n;
    }
}