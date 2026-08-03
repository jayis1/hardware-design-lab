/*
 * ahrs.h — Madgwick quaternion AHRS for Inkwell
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_AHRS_H
#define INKWELL_DRIVERS_AHRS_H

#include <stdint.h>

void  ahrs_init(float beta, uint32_t sample_hz);
void  ahrs_set_beta(float beta);
float ahrs_get_beta(void);
void  ahrs_update(float gx, float gy, float gz,
                  float ax, float ay, float az,
                  float mx, float my, float mz);
void  ahrs_get_quaternion(float q[4]);
void  ahrs_get_euler(float *roll, float *pitch, float *yaw);

#endif