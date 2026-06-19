/*
 * imu.h — BMI270 IMU Driver for Dead-Reckoning Position
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_IMU_H
#define STRATASCAN_IMU_H

#include <stdint.h>

int  imu_init(void);
void imu_read_raw(int16_t *accel, int16_t *gyro);
void imu_get_delta(float *dx, float *dy);
void imu_reset_position(void);

#endif /* STRATASCAN_IMU_H */