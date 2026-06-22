/*
 * ml_infer.h — ML inference engine header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef ML_INFER_H
#define ML_INFER_H

#include <stdint.h>
#include "../board.h"

int ml_model_load(void);
void ml_load_builtin_model(void);
uint8_t ml_infer(const feature_vector_t *features);
const char *ml_class_name(uint8_t class_id);
float ml_confidence(void);
int ml_model_update(const uint8_t *data, uint32_t offset, uint32_t len);
int ml_model_finalize(void);

#endif /* ML_INFER_H */