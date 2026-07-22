/*
 * drivers/encoder.h — wheel encoder + IMU fusion interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_ENCODER_H
#define REBARSCOPE_ENCODER_H

#include <stdint.h>

void encoder_init(void);
void encoder_reset_origin(void);
void encoder_set_scale(float s);
void encoder_update(void);
void encoder_get_position(float *x_mm, float *y_mm, float *heading_deg);
float encoder_get_total_distance_mm(void);

#endif /* REBARSCOPE_ENCODER_H */