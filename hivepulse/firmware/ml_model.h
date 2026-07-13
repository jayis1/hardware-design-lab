/*
 * ml_model.h — On-device neural network for colony state classification
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef ML_MODEL_H
#define ML_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include "dsp.h"

/* Colony state classification labels */
typedef enum {
    COLONY_QUEENRIGHT_HEALTHY = 0,   /* Normal, queen present, active */
    COLONY_QUEENLESS          = 1,   /* Queen lost — "queenless roar" */
    COLONY_PREPARING_SWARM    = 2,   /* 5-10 days before swarm */
    COLONY_VARROA_HIGH        = 3,   /* High Varroa mite load */
    COLONY_WINTER_CLUSTER     = 4,   /* Cold-weather cluster active */
    COLONY_DEAD_INACTIVE      = 5,   /* Dead or deeply inactive */
    COLONY_STATE_COUNT
} colony_state_t;

/* Network architecture: 32 -> 24 -> 16 -> 6 (int8) */
#define ML_INPUT_SIZE    NUM_FEATURES    /* 32 */
#define ML_HIDDEN1_SIZE  24
#define ML_HIDDEN2_SIZE  16
#define ML_OUTPUT_SIZE   COLONY_STATE_COUNT  /* 6 */

/* Initialize the ML model (load int8 weights from flash) */
int ml_model_init(void);

/* Run inference on a feature vector
 * Returns the predicted colony state and confidence (0.0-1.0) */
colony_state_t ml_predict(const feature_vector_t *features, float *confidence);

/* Get the raw output logits (for debugging / BLE display) */
void ml_get_logits(int8_t logits[ML_OUTPUT_SIZE]);

/* Get human-readable state name */
const char *ml_state_name(colony_state_t state);

/* Get recommended action for a state */
const char *ml_recommended_action(colony_state_t state);

#endif /* ML_MODEL_H */