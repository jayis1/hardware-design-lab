/*
 * drivers/edgeai.h — Edge AI neural network inference for pest classification
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef EDGEAI_H
#define EDGEAI_H

#include <stdint.h>
#include "acoustic.h"
#include "spectral.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AI_NUM_CLASSES   (SPECIES_COUNT + 1)  /* 60 species + "no pest" */
#define AI_INPUT_DIM     60  /* 24 acoustic + 36 spectral */
#define AI_LAYER1_NEURONS 128
#define AI_LAYER2_NEURONS 64
#define AI_LAYER3_NEURONS 32

typedef struct {
    float class_probabilities[AI_NUM_CLASSES];
    uint8_t  top_class;
    float  top_confidence;
    float  top3_probs[3];
    uint8_t  top3_classes[3];
} ai_result_t;

int  ai_init(void);
int  ai_infer(const acoustic_features_t *acoustic,
              const spectral_features_t *spectral,
              ai_result_t *result);
int  ai_load_model(void);

/* Severity classification based on species + confidence + damage area */
uint8_t ai_classify_severity(uint8_t species_id, float confidence,
                              float damage_area_pct);

#ifdef __cplusplus
}
#endif

#endif /* EDGEAI_H */