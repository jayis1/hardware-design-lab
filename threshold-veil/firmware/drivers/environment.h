/*
 * Threshold Veil environment driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_ENVIRONMENT_H
#define THRESHOLD_VEIL_ENVIRONMENT_H

#include "board.h"

void env_init(tv_env_frame_t *frame);
void env_sample(tv_env_frame_t *frame, tv_mode_t mode, unsigned tick);
float env_pressure_trend(const tv_env_frame_t *frame);
float env_contaminant_delta(const tv_env_frame_t *frame);
float env_draft_delta(const tv_env_frame_t *frame);

#endif
