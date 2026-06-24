/*
 * dsp_fft.c — CMSIS-DSP FFT wrapper and windowing
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Wraps the CMSIS-DSP arm_rfft_fast_f32 (radix-2, 2048-point real FFT)
 * for use in the harmonic analysis pipeline.  In a production build this
 * links against the CMSIS-DSP library; the implementation here provides
 * a self-contained radix-2 FFT for compilation without external deps.
 */

#include "dsp_fft.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* CMSIS-DSP instance (in production, use arm_rfft_fast_init_f32) */
typedef struct {
    int fftLen;
    int ifftFlagR;
    int bitReverseFlagR;
    /* Twiddle factors would be precomputed here */
    float twiddle_coefs[FFT_SIZE]; /* simplified */
} rfft_instance_t;

static rfft_instance_t fft_inst;

/* ========================================================================
 * Bit-reversal table for 2048-point FFT
 * ======================================================================== */
static uint16_t bit_rev_table[FFT_SIZE];

static void build_bit_rev_table(int n) {
    int bits = 0;
    int tmp = n - 1;
    while (tmp > 0) { bits++; tmp >>= 1; }

    for (int i = 0; i < n; i++) {
        int rev = 0;
        int x = i;
        for (int j = 0; j < bits; j++) {
            rev = (rev << 1) | (x & 1);
            x >>= 1;
        }
        bit_rev_table[i] = (uint16_t)rev;
    }
}

/* ========================================================================
 * Twiddle factor precomputation
 * ======================================================================== */
static void build_twiddles(int n) {
    for (int i = 0; i < n / 2; i++) {
        float angle = -2.0f * (float)M_PI * (float)i / (float)n;
        /* Store as interleaved cos, sin pairs */
        fft_inst.twiddle_coefs[i * 2]     = cosf(angle);
        fft_inst.twiddle_coefs[i * 2 + 1] = sinf(angle);
    }
}

/* ========================================================================
 * Initialize FFT instance
 * ======================================================================== */
void dsp_fft_init(void) {
    fft_inst.fftLen = FFT_SIZE;
    fft_inst.ifftFlagR = 0;
    fft_inst.bitReverseFlagR = 1;
    build_bit_rev_table(FFT_SIZE);
    build_twiddles(FFT_SIZE);
}

/* ========================================================================
 * In-place radix-2 Cooley-Tukey FFT (complex, for use in real-FFT split)
 * This is a self-contained implementation.  In production, replace with
 * arm_rfft_fast_f32 for ~10× speed improvement.
 *
 * input:  [re0, im0, re1, im1, ...]  (2*N floats)
 * output: same format, in-place
 * ======================================================================== */
static void fft_complex(float *data, int n, int inverse) {
    /* Bit-reversal permutation */
    for (int i = 0; i < n; i++) {
        int j = bit_rev_table[i < FFT_SIZE ? i : 0];
        if (j > i) {
            float tr = data[i * 2];
            float ti = data[i * 2 + 1];
            data[i * 2]     = data[j * 2];
            data[i * 2 + 1] = data[j * 2 + 1];
            data[j * 2]     = tr;
            data[j * 2 + 1] = ti;
        }
    }

    /* Butterfly stages */
    for (int len = 2; len <= n; len <<= 1) {
        float angle = (inverse ? 2.0f : -2.0f) * (float)M_PI / (float)len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);

        for (int i = 0; i < n; i += len) {
            float w_r = 1.0f, w_i = 0.0f;
            for (int j = 0; j < len / 2; j++) {
                float u_r = data[(i + j) * 2];
                float u_i = data[(i + j) * 2 + 1];
                float v_r = data[(i + j + len / 2) * 2] * w_r
                           - data[(i + j + len / 2) * 2 + 1] * w_i;
                float v_i = data[(i + j + len / 2) * 2] * w_i
                           + data[(i + j + len / 2) * 2 + 1] * w_r;

                data[(i + j) * 2]             = u_r + v_r;
                data[(i + j) * 2 + 1]         = u_i + v_i;
                data[(i + j + len / 2) * 2]     = u_r - v_r;
                data[(i + j + len / 2) * 2 + 1] = u_i - v_i;

                /* Rotate w */
                float nw_r = w_r * wlen_r - w_i * wlen_i;
                w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = nw_r;
            }
        }
    }
}

/* ========================================================================
 * Real-valued FFT: pack real input into complex, run, unpack
 *
 * input:  N real floats
 * output: N floats (packed: [re0, re(N/2), re1, im1, re2, im2, ...])
 *         We output full complex (2N floats) for simplicity of downstream code.
 * ======================================================================== */
void dsp_fft_run(float *input, float *output) {
    int n = FFT_SIZE;
    float complex_buf[FFT_SIZE * 2];

    /* Pack real input into complex (imaginary = 0) */
    for (int i = 0; i < n; i++) {
        complex_buf[i * 2]     = input[i];
        complex_buf[i * 2 + 1] = 0.0f;
    }

    /* Run complex FFT */
    fft_complex(complex_buf, n, 0);

    /* Copy to output */
    for (int i = 0; i < n * 2; i++) {
        output[i] = complex_buf[i];
    }
}

/* ========================================================================
 * Compute magnitude spectrum from complex FFT output
 * mag[k] = sqrt(re[k]^2 + im[k]^2) for k = 0..n_bins-1
 * ======================================================================== */
void dsp_fft_magnitude(float *fft_out, float *mag, int n_bins) {
    for (int k = 0; k < n_bins && k < FFT_SIZE; k++) {
        float re = fft_out[k * 2];
        float im = fft_out[k * 2 + 1];
        mag[k] = sqrtf(re * re + im * im);
    }
}