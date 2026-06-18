/*
 * classifier.c - int8 quantized CNN (MobileNetV2-0.35) particle classifier
 * Runs via CMSIS-NN. Weights live in model_weights.c as int8 arrays.
 * Input: 96x96 grayscale ROI -> 3-channel (replicated) -> 40 taxa logits.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "classifier.h"
#include "board.h"
#include "model_weights.h"
#include <string.h>
#include <math.h>

static int s_aux_uv = 0;

void classifier_init(void)
{
    /* In a production build this calls arm_nn_init_tables() and loads
     * the CMSIS-NN context from g_model_weights. */
    s_aux_uv = 0;
}

void classifier_set_aux_uv(int on)
{
    s_aux_uv = on;
}

/* Softmax over logits */
static void softmax(float *v, int n)
{
    float mx = v[0];
    for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
    float s = 0.0f;
    for (int i = 0; i < n; i++) { v[i] = expf(v[i] - mx); s += v[i]; }
    for (int i = 0; i < n; i++) v[i] /= s;
}

/* Minimal depthwise-separable conv2d with int8 weights (no CMSIS-NN dep):
 * Used as a reference implementation; production swaps in arm_convolve_s8.
 * For brevity we implement a plain (non-depthwise) conv2d followed by ReLU. */
static void conv2d_s8_ref(const int8_t *in, int W, int H, int Cin,
                          const int8_t *w, const int32_t *b,
                          int8_t *out, int OW, int OH, int Cout,
                          int k, int stride)
{
    for (int co = 0; co < Cout; co++) {
        for (int oy = 0; oy < OH; oy++) {
            for (int ox = 0; ox < OW; ox++) {
                int32_t acc = b[co];
                for (int ci = 0; ci < Cin; ci++) {
                    for (int ky = 0; ky < k; ky++) {
                        for (int kx = 0; kx < k; kx++) {
                            int ix = ox * stride + kx;
                            int iy = oy * stride + ky;
                            if (ix < 0 || iy < 0 || ix >= W || iy >= H) continue;
                            int32_t a = in[(iy * W + ix) * Cin + ci];
                            int32_t ww = w[((co * Cin + ci) * k + ky) * k + kx];
                            acc += a * ww;
                        }
                    }
                }
                /* ReLU + requantize to int8 (scale=1/64) */
                if (acc < 0) acc = 0;
                acc >>= 6;
                if (acc > 127) acc = 127;
                out[(oy * OW + ox) * Cout + co] = (int8_t)acc;
            }
        }
    }
}

/* Global-average-pool over a (H,W,C) int8 tensor -> (C,) int8 */
static void gap_s8(const int8_t *in, int W, int H, int C, int8_t *out)
{
    for (int c = 0; c < C; c++) {
        int32_t s = 0;
        for (int i = 0; i < W * H; i++) s += in[i * C + c];
        out[c] = (int8_t)(s / (W * H));
    }
}

/* Dense (fully-connected) int8 -> float logits */
static void dense_s8(const int8_t *in, int Cin,
                     const int8_t *w, const int32_t *b,
                     float *out, int Cout)
{
    for (int co = 0; co < Cout; co++) {
        int32_t acc = b[co];
        for (int ci = 0; ci < Cin; ci++) acc += in[ci] * w[co * Cin + ci];
        out[co] = (float)acc / 4096.0f;   /* dequantize (scale ~1/4096) */
    }
}

int classifier_predict(const roi_t *roi, float *logits)
{
    /* Stage 1: expand grayscale to 3 channels (96x96x3) */
    static int8_t input[ROI_MAX_DIM * ROI_MAX_DIM * 3];
    for (int i = 0; i < ROI_MAX_DIM * ROI_MAX_DIM; i++) {
        int8_t v = (int8_t)roi->pixels[i] - 128;   /* center */
        input[i * 3 + 0] = v;
        input[i * 3 + 1] = v;
        input[i * 3 + 2] = v;
    }
    /* If UV aux channel is selected, shift the B channel by a bias to
     * encode fluorescence (simplified). */
    if (s_aux_uv) {
        for (int i = 0; i < ROI_MAX_DIM * ROI_MAX_DIM; i++)
            input[i * 3 + 2] += 16;
    }

    /* Stage 2: 3 conv blocks (reduced MobileNetV2 stem for firmware demo) */
    static int8_t buf1[48 * 48 * 16];
    conv2d_s8_ref(input, ROI_MAX_DIM, ROI_MAX_DIM, 3,
                  g_conv0_w, g_conv0_b, buf1, 48, 48, 16, 3, 2);

    static int8_t buf2[24 * 24 * 32];
    conv2d_s8_ref(buf1, 48, 48, 16,
                  g_conv1_w, g_conv1_b, buf2, 24, 24, 32, 3, 2);

    static int8_t buf3[12 * 12 * 64];
    conv2d_s8_ref(buf2, 24, 24, 32,
                  g_conv2_w, g_conv2_b, buf3, 12, 12, 64, 3, 2);

    /* Stage 3: GAP -> dense -> logits */
    int8_t pooled[64];
    gap_s8(buf3, 12, 12, 64, pooled);
    dense_s8(pooled, 64, g_fc_w, g_fc_b, logits, TAXA_CLASSES);
    softmax(logits, TAXA_CLASSES);

    /* Argmax */
    int best = 0;
    for (int i = 1; i < TAXA_CLASSES; i++)
        if (logits[i] > logits[best]) best = i;
    return best;
}

const char *classifier_class_name(int idx)
{
    if (idx < 0 || idx >= TAXA_CLASSES) return "unknown";
    return g_class_names[idx];
}