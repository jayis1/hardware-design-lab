/*
 * inference.c — CMSIS-NN int8 state classifier for lichen colony health.
 *
 * A small multilayer perceptron (6 → 24 → 16 → 5) implemented as a
 * straightforward int8 matrix-vector multiply + requantize. The weights
 * are in model.c (authored by jayis1). This is a hand-rolled inference
 * engine rather than the full CMSIS-NN library, so it has no external
 * dependency and compiles cleanly in CI.
 *
 * Author: jayis1
 * License: MIT
 */

#include "inference.h"
#include "model.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/* Quantization helpers                                                      */
/* ------------------------------------------------------------------------ */
/* Input features are quantized to int8 using per-feature scales. */
static const float s_input_scales[6] = {
    127.0f / 0.9f,    /* fv_fm ∈ [0, 0.9]          */
    127.0f / 1.0f,    /* lndvi ∈ [-1, 1]            */
    127.0f / 3.0f,    /* chl_index ∈ [0, ~3]        */
    127.0f / 1.0f,    /* wetness ∈ [0, 1]           */
    127.0f / 64.0f,   /* acoustic_events ∈ [0, ~64] */
    127.0f / 12.0f,   /* uv_index ∈ [0, 12]         */
};

static const float s_input_zp[6] = { 0, 0, 0, 0, 0, 0 };

/* ------------------------------------------------------------------------ */
/* Int8 matmul:  out[n] = bias[n] + sum_i (W[n][i] * in[i])                    */
/* Followed by requantization via output scale / zero point.                  */
/* ------------------------------------------------------------------------ */
static void linear_i8(const int8_t *W, const int32_t *bias,
                       int8_t *out, int in_dim, int out_dim,
                       const int8_t *in, float out_scale, int8_t out_zp)
{
    for (int n = 0; n < out_dim; n++) {
        int32_t acc = bias[n];
        const int8_t *row = &W[n * in_dim];
        for (int i = 0; i < in_dim; i++) {
            acc += (int32_t)row[i] * (int32_t)in[i];
        }
        /* Requantize: float = acc * scale; int8 = clamp(round(float) + zp). */
        float v = (float)acc * out_scale + (float)out_zp;
        int iv = (int)(v + 0.5f);
        if (iv < -128) iv = -128;
        if (iv > 127)  iv = 127;
        out[n] = (int8_t)iv;
    }
}

/* ------------------------------------------------------------------------ */
/* ReLU activation                                                            */
/* ------------------------------------------------------------------------ */
static void relu_i8(int8_t *buf, int n)
{
    for (int i = 0; i < n; i++) {
        if (buf[i] < 0) buf[i] = 0;
    }
}

/* ------------------------------------------------------------------------ */
/* Softmax over int8 logits → confidence                                      */
/* ------------------------------------------------------------------------ */
static uint8_t softmax_argmax(const int8_t *logits, int n, uint8_t *conf)
{
    int8_t max = logits[0];
    int idx = 0;
    for (int i = 1; i < n; i++) {
        if (logits[i] > max) { max = logits[i]; idx = i; }
    }
    /* exp approximation scaled by max for stability. */
    float sum = 0.0f;
    float exps[5];
    for (int i = 0; i < n; i++) {
        float diff = (float)(logits[i] - max) * 0.05f;  /* temperature */
        /* exp via simple approximation: 1 + x + x²/2 for x near 0 (clipped). */
        if (diff < -6.0f) diff = -6.0f;
        float e = 1.0f + diff + (diff * diff) * 0.5f;
        if (e < 0.0001f) e = 0.0001f;
        exps[i] = e;
        sum += e;
    }
    float p = exps[idx] / sum;
    *conf = (uint8_t)(p * 100.0f);
    return (uint8_t)idx;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
uint8_t inference_run(const inference_features_t *f, uint8_t *confidence)
{
    if (f == NULL || confidence == NULL) return INFERENCE_STATE_DORMANT;

    /* 1. Quantize inputs. */
    int8_t in0[6];
    in0[0] = (int8_t)((f->fv_fm - s_input_zp[0]) * s_input_scales[0]);
    in0[1] = (int8_t)((f->lndvi - s_input_zp[1]) * s_input_scales[1]);
    in0[2] = (int8_t)((f->chl_index - s_input_zp[2]) * s_input_scales[2]);
    in0[3] = (int8_t)((f->wetness - s_input_zp[3]) * s_input_scales[3]);
    in0[4] = (int8_t)((f->acoustic_events - s_input_zp[4]) * s_input_scales[4]);
    in0[5] = (int8_t)((f->uv_index - s_input_zp[5]) * s_input_scales[5]);

    /* 2. Layer 1: 6 → 24 with ReLU. */
    int8_t h1[24];
    linear_i8(model_w1, model_b1, h1, 6, 24, in0, model_scale1, model_zp1);
    relu_i8(h1, 24);

    /* 3. Layer 2: 24 → 16 with ReLU. */
    int8_t h2[16];
    linear_i8(model_w2, model_b2, h2, 24, 16, h1, model_scale2, model_zp2);
    relu_i8(h2, 16);

    /* 4. Output layer: 16 → 5 (no activation; softmax argmax below). */
    int8_t logits[5];
    linear_i8(model_w3, model_b3, logits, 16, 5, h2, model_scale3, model_zp3);

    /* 5. Softmax → class + confidence. */
    return softmax_argmax(logits, 5, confidence);
}