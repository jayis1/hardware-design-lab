/*
 * moisture.h — StudGuard segmented capacitive moisture sensing
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_MOISTURE_H
#define STUDGUARD_MOISTURE_H

#include <stdint.h>
#include "../board.h"

typedef struct {
    float segments[CAP_SEGMENT_COUNT];
    float mean;
    float delta;
    float vector_x;
    float vector_y;
    float baseline_mean;
    float baseline_segments[CAP_SEGMENT_COUNT];
} moisture_state_t;

void moisture_init(moisture_state_t *state);
void moisture_sample(moisture_state_t *state, uint32_t tick, float humidity_rh, float thermal_bias);
void moisture_update_baseline(moisture_state_t *state, uint32_t baseline_count);
float moisture_score(const moisture_state_t *state, float *spread_score);
void moisture_direction_text(const moisture_state_t *state, char *buffer, uint32_t buffer_len);

#endif
