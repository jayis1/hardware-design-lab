/*
 * Threshold Veil power driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_POWER_H
#define THRESHOLD_VEIL_POWER_H

#include "board.h"

void power_init(tv_power_frame_t *frame);
void power_step(tv_power_frame_t *frame, const tv_seal_frame_t *seal, const tv_env_frame_t *env, tv_mode_t mode);

#endif
