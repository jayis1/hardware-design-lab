/*
 * Threshold Veil seal driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_SEAL_H
#define THRESHOLD_VEIL_SEAL_H

#include "board.h"

void seal_init(tv_seal_frame_t *frame);
void seal_apply(tv_seal_frame_t *frame, const tv_inference_t *inf, const tv_env_frame_t *env, tv_mode_t mode);
float seal_effectiveness(const tv_seal_frame_t *frame, const tv_env_frame_t *env);

#endif
