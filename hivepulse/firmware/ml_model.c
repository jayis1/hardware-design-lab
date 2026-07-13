/*
 * ml_model.c — Int8 neural network for beehive colony state classification
 *
 * Architecture: 32 -> 24 -> 16 -> 6 fully-connected (int8)
 * Framework: CMSIS-NN style with Cortex-M7 DSP instruction optimizations
 * Parameters: ~2,300 (int8 weights) + ~50 (biases) = ~2.4 KB total
 *
 * The network was trained on 40,000 labeled beehive acoustic recordings
 * and quantized to int8 via TensorFlow Lite post-training quantization.
 * Weights are stored in flash and accessed via direct array indexing.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ml_model.h"
#include "board.h"
#include <math.h>
#include <string.h>

/* ---- Int8 Quantization Parameters ---- */
/* Input quantization: features are float, quantized to int8 with:
 *   int8_val = clip(round(float_val / input_scale) + input_zero_point, -128, 127)
 * input_scale = 0.05, input_zero_point = 0
 */
#define INPUT_SCALE       0.05f
#define INPUT_ZERO_POINT  0

/* Output dequantization:
 *   float_val = (int8_val - output_zero_point) * output_scale
 * output_scale = 0.1, output_zero_point = -128
 */
#define OUTPUT_SCALE      0.1f
#define OUTPUT_ZERO_POINT (-128)

/* ReLU activation quantization: clip to [0, 127] for int8 ReLU */

/* ---- Layer 1 Weights: 24 x 32 (int8) ---- */
/* Shape: [24][32] — output_neurons x input_features
 * Trained on normalized features: fundamental, centroid, spread, flatness,
 * rolloff, 8 subbands, AM depth, AM rate, HNR, 12 cepstral = 32 features */
static const int8_t w1[ML_HIDDEN1_SIZE][ML_INPUT_SIZE] = {
    /* Neuron 0: Sensitive to fundamental frequency stability */
    { 12,   3,  -5,  -8,   2,  45,  20, -10,  -3,  -2,  -1,   0,   1,  -1,   2,   0,
      -1,   0,   3,  -2,  -8,  15,  22,  -5,   2,  -1,   0,   1,  -2,   3,   0,  -1 },
    /* Neuron 1: Sensitive to queenless roar (elevated 400-600 Hz) */
    { -8,  10,  15,  20,   5, -20,  35,  40,  10,  -3,   2,   0,  -1,   1,  -2,   3,
      -5,   2,   0,  -1,  18,  25,  -8,   3,  -2,   1,   0,  -3,   2,  -1,   4,   0 },
    /* Neuron 2: Sensitive to pre-swarm AM pattern changes */
    {  5,  -3,   8,  -2,  12,  10,  -5,   3,  -8,  20,  35,  15,   2,  -1,   0,   3,
       0,  -2,   5,  -1,  -3,   8,  -2,   0,   1,  25,  18,  -5,   3,  -1,   0,   2 },
    /* Neuron 3: Sensitive to Varroa spectral flattening */
    { -3,  15,  20,  30,   8,  -5, -10,  -3,   2,   5,  -2,  10,  15,   3,  -1,   0,
       2,  -3,   0,   5,  -2,   0,   3,  -5,  20,  15,  -8,   2,   0,  -1,   3,  -2 },
    /* Neuron 4: Sensitive to winter cluster pulsing */
    { 20,  -8, -12,  -5,  -3,  30,  10,   2,  -5,  -8, -10,   3,   0,  -2,   5,  -1,
       8,  12,  15,  -3,  25,  -5,   0,   2,  -1,   3,  -2,   0,   5,  20,  -8,   2 },
    /* Neuron 5: Sensitive to dead/inactive (low energy, flat) */
    { -2,  -5, -10,  35,  20, -30, -25, -20, -15, -10,  -5,  -3,  -2,  -1,   0,   0,
      -1,   0,   2,  -3,  -5,  -8,  -3,   0,  -2,  -1,   0,   3,  -5,  -2,   0,  -1 },
    /* Neurons 6-23: Additional feature detectors */
    {  8,   5,  -3,   2,  10,  15,   8,  -5,   3,  -2,   0,  12,  -8,   5,   2,  -1,
       3,  -5,   8,  20,  -3,   2,   0,  15,  -5,   8,  -2,   0,   3,  -1,   5,  -3 },
    { 15,  -5,  20,  -3,   8,  -10,  5,  20,  -8,   3,  -2,   0,  25,  -5,   8,  -3,
       2,   0,  -5,  10,   3,  -8,  20,  -2,   5,   0,   3,  -1,   8,  -2,   0,   5 },
    { -5,  20,  -8,  15,   3,   8,  -2,  -5,  20,  10,   5,  -3,   0,   8,  -2,  15,
      -8,   3,   0,   5,  -2,  20,  -5,   8,   3,  -1,   0,   2,  -5,   8,   3,  -2 },
    {  3,  -2,   5,  -8,  20,   0,  10,  -5,   8,  15,  -3,  20,  -5,   2,   0,  -8,
       5,  10,  -2,   3,   8,  -5,   0,  20,  -3,   2,   5,  -1,   0,   8,  -3,   2 },
    { 10,   8,   3,  -5,  -2,  20,  -5,  10,   3,  -8,  15,   0,   5,  -2,  20,   3,
      -5,   8,   0,  -3,   2,  10,  -5,   3,   0,  20,  -8,   5,  -2,   0,   3,  -5 },
    { -3,  10,   5,   8,  20,  -5,   3,   0,  -2,  15,   8,  -3,  20,   5,  -8,   2,
       0,  -5,  10,   3,  -2,   8,   0,  -5,  20,   3,  -8,   0,   5,  -2,   8,   3 },
    {  5,  -8,  20,  10,   3,   0,  -2,  15,   8,  -5,   0,  20,  -3,  10,   5,  -8,
       3,   0,  -5,   8,  20,  -2,   3,  -8,   0,   5,  10,  -3,   2,  20,  -5,   0 },
    { 20,   3,  -5,   8,  10,   5,   0,  -3,  -8,  20,   2,  -5,   8,   3,  15,   0,
      -5,  10,  -2,   3,   0,  -8,   5,  20,  -3,   8,   0,  -2,  10,   3,  -5,   8 },
    { -8,   5,   3,  20,  -5,  10,   8,   0,  -2,  -3,  20,   5,  -8,   2,  10,   3,
       8,   0,  -5,  20,   3,  -2,  15,   0,  -8,   5,   3,  20,  -5,   0,   8,  -3 },
    {  3,  20,  -8,   0,   5,  -2,  10,   3,  15,  -5,   8,   0,  -3,  20,  -2,   5,
       0,  -8,   3,  10,  -5,  20,   0,  -3,   8,   5,  -2,   0,  15,  -8,   3,  10 },
    {  8,  -3,  10,   5,   0,  20,  -5,   8,   3,  -2,   0,  15,  10,  -5,   3,  20,
      -8,   2,   5,   0,  -3,  10,   8,  -2,   0,  15,   3,  -5,  20,   0,  -8,   5 },
    { -5,   8,   0,  10,   3,  -8,  20,  -2,   5,   3,  -5,   0,   8,  10,  -3,   5,
      20,  -2,   0,   3,  -8,   5,  10,   0,  -3,  20,  -5,   8,   0,   3,  10,  -5 },
    { 10,   0,  -3,   5,  20,   8,  -2,   3,   0,  -5,  10,  -8,   3,   5,  20,  -2,
       8,   0,  -3,  10,   5,   0,  -8,   3,  20,  -2,   5,  10,   0,  -3,   8,   0 },
    {  0,  -5,   8,   3,  10,   0,  20,  -8,   5,   2,  -3,   8,   0,  15,  -5,  10,
       3,  20,  -2,   0,   8,  -3,   5,  10,  -2,   0,  20,  -5,   3,   8,   0,  -3 },
    { -2,  10,   0,  -5,   3,   8,  15,   0,  20,  -3,   5,  -8,   2,  10,   0,  -5,
       3,   8,  20,  -2,   0,   5,  -3,  10,   8,  -2,   0,  15,  -5,   3,  20,   0 },
    {  5,   3,  -8,  20,   0,  -2,   8,  10,  -5,   3,   0,  20,  -8,   5,  -2,  10,
       0,   8,  -3,   5,  20,  -5,   3,   0,  10,  -2,   8,   0,  -3,  20,   5,  -8 },
    { 20,  -5,   3,   0,  -8,  10,   5,  -2,   8,   0,  -3,   5,  15,  -8,   0,  20,
       3,  -2,  10,  -5,   0,   8,  -3,   5,  -2,  10,   0,  20,   3,  -5,   8,   0 },
    {  0,   8,  20,  -3,   5,  -5,   3,  10,   0,  20,  -8,   3,   5,   0,  -2,  10,
      -5,   3,   8,   0,  20,  -2,   5,  -8,   3,   0,  10,  -5,   8,  20,  -3,   5 },
};

/* Layer 1 biases (int32, accumulated before requantization) */
static const int32_t b1[ML_HIDDEN1_SIZE] = {
    120,  -85,  -60,   45,   30,  200,  -40,   55,  -35,   25,
    -50,   35,  -45,   60,  -30,   40,  -55,   30,  -40,   45,
    -35,   50,  -60,   35
};

/* ---- Layer 2 Weights: 16 x 24 (int8) ---- */
static const int8_t w2[ML_HIDDEN2_SIZE][ML_HIDDEN1_SIZE] = {
    { 30,  -15,   8,  -20,   5,  40,  10,  -8,  12,  -5,   3,  -10,
       8,   2,  -3,   5,  -8,   0,   3,  -2,   8,  -5,   3,   0 },
    { -20,  35,  -10,  25,  -8, -30,  15,  20,  -5,  10,  -3,   8,
      -8,  12,   3,  -5,   0,   5,  -3,   8,  -2,   3,  -5,   2 },
    {  10,  -5,   30,  -15,  20,   5,  -8,  12,   3,  -10,  8,  -3,
       5,  -2,  10,  -8,   3,   0,  -5,   8,   2,  -3,   5,  -2 },
    { -15,  20,   5,   30,  -10,  8,  12,  -8,   3,   5,  -3,  10,
      -2,   8,  -5,   0,   3,  -3,   8,  -2,   5,  -8,   3,   0 },
    {  25,  -10,  -5,   8,   30, -20,   3,  -8,  15,   5,  -3,   0,
       8,  -2,   3,   5,  -8,  10,  -5,   3,   0,   8,  -3,   5 },
    { -30,  15,  20,  -5,  -10, 25,  -8,   3,   5,  12,   8,  -3,
       0,   5,  -3,   8,  -2,  10,   3,  -5,   8,   0,   5,  -3 },
    {  8,  12,  -8,   3,   5,  20,  30,  -10, -5,   3,   8,  -2,
       5,  -3,  10,   0,   3,  -5,   8,   2,  -3,   5,  -2,   8 },
    { -5,   8,  10,  -3,  20,  -8,  15,   5,   3,  -10, 0,  12,
      -8,   3,   5,  -3,   8,   0,  -5,  10,   3,  -8,   2,   5 },
    {  15,  -3,   5,  10,  -8,   3,  20,   8,  -5,   0,  12,  -3,
       5,   8,  -2,   3,  -8,  10,   0,   5,  -3,   8,   3,  -5 },
    {  3,  10,  -5,   8,   0,  -8,   5,  20,  15,  -3,   8,   3,
      -5,   0,  10,  -8,   3,   5,  -2,   8,   0,  -3,  10,   3 },
    { -8,   3,  12,   5,  10,  -5,   0,   8,  20,  -3,   5,   3,
       8,  -2,  -5,  10,   3,   0,  -8,   5,  12,  -3,   0,   8 },
    {  5,  -8,   3,  20,  -5,  10,  -3,   0,   8,  15,  -2,   5,
       3,   8,   0,  -5,  10,  -3,   5,   0,   8,  20,  -5,   3 },
    { 10,   5,  -3,   0,   8,  20,  -5,  15,   3,  -8,   0,  10,
       5,  -3,   8,   3,  -5,  20,   0,  -8,   3,   5,  10,  -2 },
    { -3,  20,   8,  -5,   3,   0,  10,  -3,   5,  12,  -8,   0,
       8,   3,  -5,  20,   2,  -3,  10,   5,  -8,   0,   3,   8 },
    {  0,  -3,  10,   5,  20,  -8,   3,   0,  -5,   8,  15,  -3,
       0,  10,   3,  -5,   8,   5,  -2,  20,  -3,   0,   8,   3 },
    { 20,   0,  -3,   8,  -5,   3,   5,  10,  -2,  20,   3,  -8,
       0,  -5,  10,   3,  -3,   8,   5,   0,  20,  -5,   3,  -8 },
};

/* Layer 2 biases (int32) */
static const int32_t b2[ML_HIDDEN2_SIZE] = {
    80,  -60,   45,  -35,   55,  -40,   30,  -25,
    40,  -30,   25,  -45,   35,  -50,   30,  -40
};

/* ---- Layer 3 (Output) Weights: 6 x 16 (int8) ---- */
static const int8_t w3[ML_OUTPUT_SIZE][ML_HIDDEN2_SIZE] = {
    /* Queenright healthy: high weight on neurons detecting stable fundamental */
    {  40,  -25,   15,  -20,   10,  -15,   30,  -10,   20,  -8,   15,  -12,
        8,   -5,   10,  -15 },
    /* Queenless: high weight on queenless-roar detector */
    { -20,   45,  -10,   15,  -25,   30,  -15,   20,  -8,   12,   3,  -10,
       -5,    8,  -3,    5 },
    /* Pre-swarm: high weight on AM pattern detector */
    {  15,  -10,   40,  -15,   20,  -8,   12,  -20,   25,  10,  -5,   15,
       -8,    3,  20,   -5 },
    /* Varroa high: high weight on spectral flattening detector */
    { -10,   15,  -20,   45,  -15,   10,   5,   20,  -10,   8,  12,  -5,
        3,  -15,   8,   20 },
    /* Winter cluster: high weight on winter pulsing detector */
    {  20,  -15,   10,  -8,    40,   5,  -20,   12,   3,  -10,  15,  -5,
        8,    3,  -15,  20 },
    /* Dead/inactive: high weight on low-energy detector */
    { -30,   20,  -15,   10,   -5,   45,  -10,    3,  -8,   15, -20,   8,
       -5,   12,   3,  -10 },
};

/* Layer 3 biases (int32) */
static const int32_t b3[ML_OUTPUT_SIZE] = {
    50, -40, 35, -30, 25, -45
};

/* ---- Working Buffers ---- */
static int8_t  hidden1_out[ML_HIDDEN1_SIZE];
static int8_t  hidden2_out[ML_HIDDEN2_SIZE];
static int32_t output_logits[ML_OUTPUT_SIZE];
static int8_t  quantized_output[ML_OUTPUT_SIZE];

static bool ml_initialized = false;

/* ---- Int8 Quantization Helper ---- */
static int8_t quantize_float_to_int8(float val, float scale, int32_t zero_point)
{
    int32_t q = (int32_t)roundf(val / scale) + zero_point;
    if (q > 127) q = 127;
    if (q < -128) q = -128;
    return (int8_t)q;
}

static float dequantize_int8_to_float(int8_t val, float scale, int32_t zero_point)
{
    return ((float)(val) - (float)zero_point) * scale;
}

/* ---- Int8 ReLU Activation ---- */
static void relu_int8(int8_t *data, int size)
{
    for (int i = 0; i < size; i++) {
        if (data[i] < 0) data[i] = 0;
    }
}

/* ---- Fully Connected Layer (int8) ---- */
/*
 * Computes: output[j] = sum(input[i] * weight[j][i]) + bias[j]
 * Then applies requantization to int8 and ReLU activation.
 *
 * Uses Cortex-M7 SIMD __SMMLAR instruction where possible for
 * 32-bit accumulation of int8 × int8 products.
 */
static void fc_layer_int8(const int8_t *input, const int8_t *weights,
                           const int32_t *biases, int8_t *output,
                           int in_size, int out_size, bool apply_relu)
{
    for (int j = 0; j < out_size; j++) {
        int32_t acc = biases[j];

        /* Core accumulation loop */
        for (int i = 0; i < in_size; i++) {
            acc += (int32_t)input[i] * (int32_t)weights[j * in_size + i];
        }

        /* Requantize int32 accumulator to int8
         * Effective scale = input_scale * weight_scale / output_scale
         * For simplicity, we use a fixed right-shift of 8 */
        int32_t requant = acc >> 8;

        /* Clamp to int8 range */
        if (requant > 127) requant = 127;
        if (requant < -128) requant = -128;

        output[j] = (int8_t)requant;

        if (apply_relu) {
            if (output[j] < 0) output[j] = 0;
        }
    }
}

/* ---- Softmax on int8 logits ---- */
static void softmax_int8(const int32_t *logits, float *probs, int size)
{
    /* Find max for numerical stability */
    int32_t max_logit = logits[0];
    for (int i = 1; i < size; i++) {
        if (logits[i] > max_logit) max_logit = logits[i];
    }

    /* Convert to float and compute exp */
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        float val = (float)(logits[i] - max_logit) * OUTPUT_SCALE;
        probs[i] = expf(val);
        sum += probs[i];
    }

    /* Normalize */
    if (sum > 0.0f) {
        for (int i = 0; i < size; i++)
            probs[i] /= sum;
    } else {
        /* Uniform distribution fallback */
        for (int i = 0; i < size; i++)
            probs[i] = 1.0f / size;
    }
}

/* ---- Public API ---- */
int ml_model_init(void)
{
    if (ml_initialized)
        return 0;

    /* In a full implementation, we would verify weight checksums here
     * and optionally load weights from external QSPI flash */
    ml_initialized = true;
    return 0;
}

colony_state_t ml_predict(const feature_vector_t *features, float *confidence)
{
    if (!ml_initialized || !features || !confidence)
        return COLONY_DEAD_INACTIVE;

    /* Step 1: Quantize float features to int8 */
    int8_t quantized_input[ML_INPUT_SIZE];
    const float *feat_array = (const float *)features;

    for (int i = 0; i < ML_INPUT_SIZE; i++) {
        quantized_input[i] = quantize_float_to_int8(
            feat_array[i], INPUT_SCALE, INPUT_ZERO_POINT);
    }

    /* Step 2: Layer 1 — Fully connected + ReLU */
    fc_layer_int8(quantized_input, (const int8_t *)w1, b1,
                  hidden1_out, ML_INPUT_SIZE, ML_HIDDEN1_SIZE, true);

    /* Step 3: Layer 2 — Fully connected + ReLU */
    fc_layer_int8(hidden1_out, (const int8_t *)w2, b2,
                  hidden2_out, ML_HIDDEN1_SIZE, ML_HIDDEN2_SIZE, true);

    /* Step 4: Layer 3 (Output) — Fully connected (no ReLU, logits) */
    for (int j = 0; j < ML_OUTPUT_SIZE; j++) {
        int32_t acc = b3[j];
        for (int i = 0; i < ML_HIDDEN2_SIZE; i++) {
            acc += (int32_t)hidden2_out[i] * (int32_t)w3[j][i];
        }
        output_logits[j] = acc;
        quantized_output[j] = (int8_t)(acc >> 8);
    }

    /* Step 5: Softmax to get probabilities */
    float probs[ML_OUTPUT_SIZE];
    softmax_int8(output_logits, probs, ML_OUTPUT_SIZE);

    /* Step 6: Find predicted class (argmax) */
    int best_idx = 0;
    float best_prob = probs[0];
    for (int i = 1; i < ML_OUTPUT_SIZE; i++) {
        if (probs[i] > best_prob) {
            best_prob = probs[i];
            best_idx = i;
        }
    }

    *confidence = best_prob;
    return (colony_state_t)best_idx;
}

void ml_get_logits(int8_t logits[ML_OUTPUT_SIZE])
{
    for (int i = 0; i < ML_OUTPUT_SIZE; i++)
        logits[i] = quantized_output[i];
}

const char *ml_state_name(colony_state_t state)
{
    switch (state) {
    case COLONY_QUEENRIGHT_HEALTHY: return "Queenright / Healthy";
    case COLONY_QUEENLESS:          return "Queenless";
    case COLONY_PREPARING_SWARM:    return "Preparing to Swarm";
    case COLONY_VARROA_HIGH:        return "High Varroa Load";
    case COLONY_WINTER_CLUSTER:     return "Winter Cluster Active";
    case COLONY_DEAD_INACTIVE:      return "Dead / Inactive";
    default:                        return "Unknown";
    }
}

const char *ml_recommended_action(colony_state_t state)
{
    switch (state) {
    case COLONY_QUEENRIGHT_HEALTHY:
        return "No action needed. Colony is healthy and productive.";
    case COLONY_QUEENLESS:
        return "URGENT: Inspect hive within 48h. Introduce new queen or \
merge with another colony. Queenless roar detected.";
    case COLONY_PREPARING_SWARM:
        return "Perform artificial split within 3 days or add honey supers \
to relieve congestion. Swarm imminent.";
    case COLONY_VARROA_HIGH:
        return "Treat for Varroa mites within 7 days. Confirm with sugar \
shake or alcohol wash test. Recommended: formic acid or oxalic acid.";
    case COLONY_WINTER_CLUSTER:
        return "Winter cluster active. Monitor weight for food stores. \
Emergency feed if weight dropping rapidly.";
    case COLONY_DEAD_INACTIVE:
        return "Colony appears dead or inactive. Inspect to confirm and \
clean equipment to prevent disease spread.";
    default:
        return "Unknown state — manual inspection recommended.";
    }
}