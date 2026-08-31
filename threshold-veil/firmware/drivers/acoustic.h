/*
 * Threshold Veil acoustic driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_ACOUSTIC_H
#define THRESHOLD_VEIL_ACOUSTIC_H

#include "board.h"

void acoustic_init(tv_acoustic_frame_t *frame);
void acoustic_sample(tv_acoustic_frame_t *frame, const tv_env_frame_t *env, tv_mode_t mode, unsigned tick);
float acoustic_leak_index(const tv_acoustic_frame_t *frame);

#endif
