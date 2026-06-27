/*
 * ml_model.h — Edge ML model header for tremor classification
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * int8 quantized 1D-CNN for 5-class tremor type classification.
 * Input: 16-element float feature vector.
 * Output: 5-class softmax probability.
 */

#ifndef TREMORSENSE_ML_MODEL_H
#define TREMORSENSE_ML_MODEL_H

#include <stdint.h>

/* Tremor classes */
typedef enum {
    TREMOR_PARKINSONIAN = 0,
    TREMOR_ESSENTIAL   = 1,
    TREMOR_CEREBELLAR  = 2,
    TREMOR_PHYSIOLOGICAL = 3,
    TREMOR_DRUG_INDUCED = 4,
    TREMOR_CLASS_COUNT = 5
} tremor_class_t;

/* Model layer dimensions */
#define ML_INPUT_SIZE    16
#define ML_CONV1_OUT      32
#define ML_CONV1_KERNEL   3
#define ML_CONV2_OUT      16
#define ML_CONV2_KERNEL   3
#define ML_DENSE1_OUT     64
#define ML_DENSE2_OUT     5   /* = ML_CLASS_COUNT */

/* Quantization parameters */
#define ML_INPUT_SCALE    0.05f   /* Feature quantization scale */
#define ML_INPUT_ZERO_POINT (-128)
#define ML_WEIGHT_SCALE    0.01f
#define ML_WEIGHT_ZERO_POINT 0

/* Initialize ML model (load weights from flash) */
void ml_init(void);

/* Run inference: input = float features, output = class probabilities */
void ml_infer(const float *features, int n_features);

/* Get results */
uint8_t ml_get_last_class(void);
float   ml_get_last_confidence(void);
void    ml_get_probabilities(float *probs, int n);

/* Model metadata */
const char *ml_get_class_name(uint8_t class_id);
uint32_t ml_get_model_version(void);
uint32_t ml_get_model_size_bytes(void);

/* Load model weights from SPI flash */
int ml_load_weights_from_flash(uint32_t flash_addr, uint32_t size);

#endif /* TREMORSENSE_ML_MODEL_H */