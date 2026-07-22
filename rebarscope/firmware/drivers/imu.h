/*
 * drivers/imu.h — LSM6DSO IMU interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_IMU_H
#define REBARSCOPE_IMU_H

void  imu_init(void);
float imu_get_yaw_rate_dps(void);
float imu_get_yaw_deg(void);
void  imu_reset_yaw(void);
void  imu_poll(void);

#endif /* REBARSCOPE_IMU_H */