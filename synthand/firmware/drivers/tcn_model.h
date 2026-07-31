/*
 * tcn_model.h — Temporal Convolutional Network model data and interface.
 *
 * The TCN classifies 12 gesture types from 38-feature input at 500 Hz.
 * Quantized to int8 weights and int16 activations for embedded inference.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_TCN_MODEL_H
#define SYNTHAND_TCN_MODEL_H

#include <stdint.h>
#include "board.h"
#include "drivers/signal.h"

/* Model dimensions */
#define TCN_INPUT_FEATURES   38
#define TCN_TIMESTEPS        80
#define TCN_CONV1_FILTERS    24
#define TCN_CONV1_KERNEL     7
#define TCN_CONV1_DILATION   1
#define TCN_CONV2_FILTERS    24
#define TCN_CONV2_KERNEL     7
#define TCN_CONV2_DILATION   2
#define TCN_CONV3_FILTERS    24
#define TCN_CONV3_KERNEL     7
#define TCN_CONV3_DILATION   4
#define TCN_CONV4_FILTERS    16
#define TCN_CONV4_KERNEL     3
#define TCN_CONV4_DILATION   1
#define TCN_FC1_OUT          12   /* gesture classes */
#define TCN_FC2_OUT          3    /* regression: velocity, pressure, vibrato */

/* Total weight memory: ~12 KB (int8) */
#define TCN_WEIGHT_BYTES     12288

/* Activation buffer sizes */
#define TCN_ACTIVATION_BYTES 6144

/* Initialize the TCN model (load weights from flash to RAM) */
void tcn_model_init(void);

/* Push a new feature sample into the TCN sliding window.
 * Called at 500 Hz (every 2 ms). */
void tcn_model_push_sample(const feature_vector_t *features);

/* Run TCN inference on the current sliding window.
 * Called every 20 ms (every 10th sample).
 * Outputs:
 *   logits[12]   — gesture class logits (int16, pre-softmax)
 *   regression[3]— velocity, pressure, vibrato (int16, 0-127 scale) */
void tcn_model_infer(int16_t logits[TCN_FC1_OUT],
                     int16_t regression[TCN_FC2_OUT]);

/* Softmax over logits to get probabilities (Q15 fixed-point).
 * probs[12] sums to approximately 0x7FFF. */
void tcn_model_softmax(const int16_t logits[TCN_FC1_OUT],
                       q15_t probs[TCN_FC1_OUT]);

/* Get the argmax of the probability distribution */
uint8_t tcn_model_argmax(const q15_t probs[TCN_FC1_OUT]);

/* Reset the sliding window to zeros */
void tcn_model_reset_window(void);

/* 1D causal convolution with int8 weights and int16 activations.
 * Used internally by the inference engine. */
void tcn_model_conv1d(const int8_t *weights,
                      const int8_t *biases,
                      int16_t *input,
                      int16_t *output,
                      int in_channels,
                      int out_channels,
                      int kernel_size,
                      int dilation,
                      int timesteps);

/* Fully connected layer (int8 weights, int16 activations) */
void tcn_model_fully_connected(const int8_t *weights,
                               const int8_t *biases,
                               const int16_t *input,
                               int16_t *output,
                               int in_features,
                               int out_features);

/* ReLU activation in-place */
void tcn_model_relu(int16_t *activation, int length);

/* Global average pooling over the time dimension */
void tcn_model_global_avg_pool(const int16_t *input,
                               int16_t *output,
                               int channels,
                               int timesteps);

#endif /* SYNTHAND_TCN_MODEL_H */