/*
 * tflite.h — Tiny int8 inference engine (CMSIS-NN style)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * A minimal int8 convolution network runner for the GlyphFlow
 * stroke classifier. The full TFLite Micro runtime is large; GlyphFlow
 * only needs Conv1D (depthwise), ReLU, average pool, and a fully-connected
 * head, so we implement those ops directly on flat int8 weight blobs.
 */
#ifndef GLYPHFLOW_TFLITE_H
#define GLYPHFLOW_TFLITE_H

#include <stdint.h>
#include "trajectory.h"

/* Network output: per-class logit (int16) and the predicted class index. */
typedef struct {
    int16_t logits[N_CLASSES];
    int8_t  pred_class;     /* argmax class */
    int16_t confidence_x1k; /* softmax-like confidence, 0..1000 */
} inference_result_t;

/* Run the 4-layer TCN over the trajectory channels.
 * Returns 0 on success, -1 on error. */
int tflite_classify(const trajectory_t *t, inference_result_t *r);

/* Optional: set a new weight blob (OTA update path).
 * The blob is 12.6 KB of int8 weights + bias + metadata, with a
 * 32-bit CRC at the end. Returns 0 on success. */
int tflite_load_weights(const uint8_t *blob, uint32_t len);

#endif /* GLYPHFLOW_TFLITE_H */