/*
 * breath_state.h — Breath state machine and validation header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef BREATH_STATE_H
#define BREATH_STATE_H

#include <stdint.h>
#include <string.h>
#include "sensor_array.h"
#include "../board.h"

uint8_t breath_validate(const sensor_raw_t *samples, uint32_t count,
                        calib_data_t *calib);
void breath_estimate(const sensor_raw_t *samples, uint32_t count,
                     calib_data_t *calib, feature_vector_t *features,
                     breath_result_t *result);
uint8_t breath_classify_metabolic(const breath_result_t *result);
const char *breath_quality_name(uint8_t quality);
const char *metabolic_state_name(uint8_t state);
float breath_confidence(const breath_result_t *result,
                        const feature_vector_t *features);

#endif /* BREATH_STATE_H */