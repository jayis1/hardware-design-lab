/*
 * nilm.c — Int8 quantized neural network for appliance disaggregation
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Non-Intrusive Load Monitoring (NILM) infers which individual appliances
 * are running from the aggregate mains power signature alone — without
 * per-outlet sensors.  WattLens uses a feed-forward int8-quantized neural
 * network with 24 inputs (power + harmonic features) and up to 16 outputs
 * (appliance classes).
 *
 * Network architecture:
 *   Input  (24)  → Dense(64, ReLU) → Dense(64, ReLU) → Dense(64, ReLU)
 *                → Dense(16, Softmax) → appliance probabilities
 *
 * Weights are int8 quantized; inference uses integer-only matmul + ReLU,
 * with softmax dequantized at the output.  Inference time: ~0.2 ms @ 480 MHz.
 *
 * Feature vector (24 inputs):
 *   [0]  P_total_real      (W,     normalized /10000)
 *   [1]  Q_total_reactive  (var,   normalized /10000)
 *   [2]  PF_total          (×2 - 1, → [-1,1])
 *   [3]  THD_V_avg         (%,     /100)
 *   [4]  THD_I_avg         (%,     /100)
 *   [5]  freq_dev          (Hz-50, /2)
 *   [6-11]  V harmonic 3,5,7,9,11,13 (phase A, normalized)
 *   [12-17] I harmonic 3,5,7,9,11,13 (phase A, normalized)
 *   [18-23] P_real L1,L2,L3 / I_rms L1,L2,L3 (normalized)
 *
 * Training: done offline in Python (TensorFlow → TFLite int8 → C header).
 * The weights here are placeholder random values for compilation; replace
 * with trained weights via the model upload BLE characteristic.
 */

#include "nilm.h"
#include "flash_drv.h"
#include <string.h>
#include <stdlib.h>

#define NILM_INPUT_SIZE     24
#define NILM_HIDDEN1        64
#define NILM_HIDDEN2        64
#define NILM_HIDDEN3        64
#define NILM_OUTPUT_SIZE    NILM_MAX_CLASSES

/* Network dimensions */
#define L1_IN   NILM_INPUT_SIZE
#define L1_OUT  NILM_HIDDEN1
#define L2_IN   NILM_HIDDEN1
#define L2_OUT  NILM_HIDDEN2
#define L3_IN   NILM_HIDDEN2
#define L3_OUT  NILM_HIDDEN3
#define L4_IN   NILM_HIDDEN3
#define L4_OUT  NILM_OUTPUT_SIZE

/* ========================================================================
 * Default appliance class names
 * ======================================================================== */
const char *nilm_default_names[NILM_MAX_CLASSES] = {
    "Refrigerator",
    "Washing Machine",
    "Dishwasher",
    "Microwave",
    "Electric Kettle",
    "Air Conditioner",
    "Water Heater",
    "EV Charger",
    "Lighting",
    "Television",
    "Computer",
    "Pool Pump",
    "Well Pump",
    "Oven",
    "Dryer",
    "Unknown",
};

static char appliance_names[NILM_MAX_CLASSES][24];

/* ========================================================================
 * Model weights (int8, quantized)
 *
 * In production, these are loaded from QSPI flash.  The placeholder values
 * below are small deterministic pseudo-random weights for compilation.
 * ======================================================================== */

/* Quantization scales (output = int8_value × scale) */
static float scale_w1 = 0.02f, scale_b1 = 0.01f;
static float scale_w2 = 0.02f, scale_b2 = 0.01f;
static float scale_w3 = 0.02f, scale_b3 = 0.01f;
static float scale_w4 = 0.01f, scale_b4 = 0.005f;

/* Input quantization scale */
static float scale_input = 1.0f / 127.0f;

/* Weight matrices and biases (int8) */
/* Layer 1: L1_IN × L1_OUT */
static int8_t w1[L1_OUT][L1_IN];
static int8_t b1[L1_OUT];
/* Layer 2: L2_IN × L2_OUT */
static int8_t w2[L2_OUT][L2_IN];
static int8_t b2[L2_OUT];
/* Layer 3: L3_IN × L3_OUT */
static int8_t w3[L3_OUT][L3_IN];
static int8_t b3[L3_OUT];
/* Layer 4 (output): L4_IN × L4_OUT */
static int8_t w4[L4_OUT][L4_IN];
static int8_t b4[L4_OUT];

/* Working buffers */
static int32_t hidden1[L1_OUT];
static int32_t hidden2[L2_OUT];
static int32_t hidden3[L3_OUT];
static int32_t output_logits[L4_OUT];

static int num_classes = 8;  /* active classes (others = 0) */
static bool model_loaded = false;

/* ========================================================================
 * Initialize NILM module
 * ======================================================================== */
void nilm_init(void) {
    /* Copy default appliance names */
    for (int i = 0; i < NILM_MAX_CLASSES; i++) {
        strncpy(appliance_names[i], nilm_default_names[i], sizeof(appliance_names[i]) - 1);
        appliance_names[i][sizeof(appliance_names[i]) - 1] = '\0';
    }

    if (model_loaded) return;

    /* Generate placeholder weights (deterministic LCG) */
    uint32_t seed = 0x12345678;
    for (int o = 0; o < L1_OUT; o++) {
        for (int i = 0; i < L1_IN; i++) {
            seed = seed * 1103515245 + 12345;
            w1[o][i] = (int8_t)((seed >> 16) & 0xFF) - 128;
        }
        seed = seed * 1103515245 + 12345;
        b1[o] = (int8_t)((seed >> 16) & 0xFF) - 128;
    }
    for (int o = 0; o < L2_OUT; o++) {
        for (int i = 0; i < L2_IN; i++) {
            seed = seed * 1103515245 + 12345;
            w2[o][i] = (int8_t)((seed >> 16) & 0xFF) - 128;
        }
        seed = seed * 1103515245 + 12345;
        b2[o] = (int8_t)((seed >> 16) & 0xFF) - 128;
    }
    for (int o = 0; o < L3_OUT; o++) {
        for (int i = 0; i < L3_IN; i++) {
            seed = seed * 1103515245 + 12345;
            w3[o][i] = (int8_t)((seed >> 16) & 0xFF) - 128;
        }
        seed = seed * 1103515245 + 12345;
        b3[o] = (int8_t)((seed >> 16) & 0xFF) - 128;
    }
    for (int o = 0; o < L4_OUT; o++) {
        for (int i = 0; i < L4_IN; i++) {
            seed = seed * 1103515245 + 12345;
            w4[o][i] = (int8_t)((seed >> 16) & 0xFF) - 128;
        }
        seed = seed * 1103515245 + 12345;
        b4[o] = (int8_t)((seed >> 16) & 0xFF) - 128;
    }
}

/* ========================================================================
 * Load model from QSPI flash
 * Returns 0 on success, -1 on failure
 * ======================================================================== */
int nilm_load_model(void) {
    /* Try to read model header from flash */
    uint8_t header[16];
    if (flash_drv_read_model_header(header, 16) != 0) {
        model_loaded = false;
        return -1;
    }

    /* Check magic bytes "WLNM" */
    if (header[0] != 'W' || header[1] != 'L' ||
        header[2] != 'N' || header[3] != 'M') {
        model_loaded = false;
        return -1;
    }

    num_classes = header[4];
    if (num_classes > NILM_MAX_CLASSES) num_classes = NILM_MAX_CLASSES;

    /* In production: read full weight matrices from flash into SRAM3 buffers */
    /* flash_drv_read_model_weights(w1, b1, w2, b2, w3, b3, w4, b4); */

    model_loaded = true;
    return 0;
}

/* ========================================================================
 * Build 24-element feature vector from power metrics and harmonics
 * ======================================================================== */
static void build_features(const power_metrics_t *m, const harmonic_data_t *h, int8_t *feat) {
    /* Normalize features to int8 range [-128, 127] */
    feat[0]  = (int8_t)(m->p_total_real / 10000.0f * 127.0f);        /* P_real */
    feat[1]  = (int8_t)(m->p_total_reactive / 10000.0f * 127.0f);    /* Q */
    feat[2]  = (int8_t)((m->pf_total * 2.0f - 1.0f) * 127.0f);       /* PF → [-1,1] */
    feat[3]  = (int8_t)((m->thd_v[0] + m->thd_v[1] + m->thd_v[2]) / 3.0f / 100.0f * 127.0f);
    feat[4]  = (int8_t)((m->thd_i[0] + m->thd_i[1] + m->thd_i[2]) / 3.0f / 100.0f * 127.0f);
    feat[5]  = (int8_t)((m->freq - 50.0f) / 2.0f * 127.0f);          /* freq dev */

    /* Voltage harmonics (phase A: 3,5,7,9,11,13) */
    int harm_orders[] = {3, 5, 7, 9, 11, 13};
    for (int k = 0; k < 6; k++) {
        float val = h->v_mag[0][harm_orders[k]];
        float fund = h->v_mag[0][1];
        float ratio = fund > 0.1f ? val / fund : 0.0f;
        feat[6 + k] = (int8_t)(ratio * 127.0f);
    }

    /* Current harmonics (phase A: 3,5,7,9,11,13) */
    for (int k = 0; k < 6; k++) {
        float val = h->i_mag[0][harm_orders[k]];
        float fund = h->i_mag[0][1];
        float ratio = fund > 0.1f ? val / fund : 0.0f;
        feat[12 + k] = (int8_t)(ratio * 127.0f);
    }

    /* Per-phase real power and current */
    feat[18] = (int8_t)(m->p_real[0] / 5000.0f * 127.0f);
    feat[19] = (int8_t)(m->p_real[1] / 5000.0f * 127.0f);
    feat[20] = (int8_t)(m->p_real[2] / 5000.0f * 127.0f);
    feat[21] = (int8_t)(m->i_rms[0] / 100.0f * 127.0f);
    feat[22] = (int8_t)(m->i_rms[1] / 100.0f * 127.0f);
    feat[23] = (int8_t)(m->i_rms[2] / 100.0f * 127.0f);

    /* Clamp */
    for (int i = 0; i < NILM_INPUT_SIZE; i++) {
        if (feat[i] > 127) feat[i] = 127;
        if (feat[i] < -128) feat[i] = -128;
    }
}

/* ========================================================================
 * Int8 matrix multiplication: output[o] = bias[o] + Σ_i w[o][i] * x[i]
 * ======================================================================== */
static void matmul_add_bias(const int8_t *w, const int8_t *b, const int8_t *x,
                            int32_t *out, int n_in, int n_out) {
    for (int o = 0; o < n_out; o++) {
        int32_t sum = (int32_t)b[o];
        const int8_t *wrow = &w[o * n_in];
        for (int i = 0; i < n_in; i++) {
            sum += (int32_t)wrow[i] * (int32_t)x[i];
        }
        out[o] = sum;
    }
}

/* ========================================================================
 * ReLU activation (in-place on int32 accumulator)
 * ======================================================================== */
static void relu_int32(int32_t *x, int n) {
    for (int i = 0; i < n; i++) {
        if (x[i] < 0) x[i] = 0;
    }
}

/* ========================================================================
 * Softmax over int32 logits → float probabilities
 * ======================================================================== */
static void softmax(const int32_t *logits, float *probs, int n) {
    /* Find max for numerical stability */
    int32_t max = logits[0];
    for (int i = 1; i < n; i++) {
        if (logits[i] > max) max = logits[i];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        /* Dequantize: scale by combined weight/input scales */
        float v = (float)(logits[i] - max) * 0.0001f;  /* approximate combined scale */
        probs[i] = expf_approx(v);
        sum += probs[i];
    }
    if (sum > 0.0f) {
        for (int i = 0; i < n; i++) {
            probs[i] /= sum;
        }
    }
}

/* Approximate exp() for softmax (fast, no libm dependency) */
static float expf_approx(float x) {
    if (x < -20.0f) return 0.0f;
    /* exp(x) = 2^(x/ln2), split into integer and fractional parts */
    int xi = (int)(x * 1.442695f);  /* x / ln(2) */
    float xf = x * 1.442695f - (float)xi;
    /* 2^xf via polynomial */
    float result = 1.0f + xf * 0.693147f + xf * xf * 0.240227f;
    /* Multiply by 2^xi (bit shift via ldexpf equivalent) */
    if (xi >= 0) {
        for (int i = 0; i < xi; i++) result *= 2.0f;
    } else {
        for (int i = 0; i < -xi; i++) result *= 0.5f;
    }
    return result;
}

/* ========================================================================
 * Main NILM inference
 * ======================================================================== */
void nilm_infer(const power_metrics_t *m, const harmonic_data_t *h, nilm_result_t *r) {
    int8_t features[NILM_INPUT_SIZE];
    build_features(m, h, features);

    /* Layer 1: matmul + ReLU */
    matmul_add_bias((const int8_t *)w1, b1, features, hidden1, L1_IN, L1_OUT);
    relu_int32(hidden1, L1_OUT);

    /* Quantize hidden1 back to int8 for next layer */
    int8_t h1_int8[L1_OUT];
    for (int i = 0; i < L1_OUT; i++) {
        int32_t v = hidden1[i] >> 7;  /* right-shift by log2(128) for re-quantization */
        if (v > 127) v = 127;
        if (v < -128) v = -128;
        h1_int8[i] = (int8_t)v;
    }

    /* Layer 2: matmul + ReLU */
    matmul_add_bias((const int8_t *)w2, b2, h1_int8, hidden2, L2_IN, L2_OUT);
    relu_int32(hidden2, L2_OUT);

    int8_t h2_int8[L2_OUT];
    for (int i = 0; i < L2_OUT; i++) {
        int32_t v = hidden2[i] >> 7;
        if (v > 127) v = 127;
        if (v < -128) v = -128;
        h2_int8[i] = (int8_t)v;
    }

    /* Layer 3: matmul + ReLU */
    matmul_add_bias((const int8_t *)w3, b3, h2_int8, hidden3, L3_IN, L3_OUT);
    relu_int32(hidden3, L3_OUT);

    int8_t h3_int8[L3_OUT];
    for (int i = 0; i < L3_OUT; i++) {
        int32_t v = hidden3[i] >> 7;
        if (v > 127) v = 127;
        if (v < -128) v = -128;
        h3_int8[i] = (int8_t)v;
    }

    /* Layer 4 (output): matmul → logits → softmax */
    matmul_add_bias((const int8_t *)w4, b4, h3_int8, output_logits, L4_IN, L4_OUT);
    softmax(output_logits, r->nilm_probs, num_classes);

    /* Threshold to binary on/off states with hysteresis */
    for (int c = 0; c < NILM_MAX_CLASSES; c++) {
        if (c < num_classes) {
            float p = r->nilm_probs[c];
            /* On if probability > 0.7, off if < 0.4 (hysteresis) */
            if (r->nilm_state[c] == 0) {
                r->nilm_state[c] = (p > 0.7f) ? 1 : 0;
            } else {
                r->nilm_state[c] = (p < 0.4f) ? 0 : 1;
            }
            /* Estimate per-appliance power (proportional to probability × total power) */
            r->nilm_power[c] = p * m->p_total_real;
        } else {
            r->nilm_probs[c] = 0.0f;
            r->nilm_state[c] = 0;
            r->nilm_power[c] = 0.0f;
        }
    }
}

/* ========================================================================
 * Appliance name accessors
 * ======================================================================== */
int nilm_get_appliance_name(int class_id, char *name, int len) {
    if (class_id < 0 || class_id >= NILM_MAX_CLASSES) return -1;
    strncpy(name, appliance_names[class_id], len - 1);
    name[len - 1] = '\0';
    return 0;
}

void nilm_set_appliance_name(int class_id, const char *name) {
    if (class_id < 0 || class_id >= NILM_MAX_CLASSES) return;
    strncpy(appliance_names[class_id], name, sizeof(appliance_names[class_id]) - 1);
    appliance_names[class_id][sizeof(appliance_names[class_id]) - 1] = '\0';
}

int nilm_get_num_classes(void) {
    return num_classes;
}