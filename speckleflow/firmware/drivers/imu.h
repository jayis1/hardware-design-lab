/*
 * imu.h — ICM-42688-P 6-axis IMU driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_IMU_H
#define SPECKLEFLOW_IMU_H

#include <stdint.h>

typedef struct {
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int8_t  temp_c;
} imu_sample_t;

/**
 * Initialize the ICM-42688-P IMU.
 * @return 0 on success, -1 on I2C error, -2 on wrong chip ID
 */
int imu_init(void);

/**
 * Read a 6-axis sample (accel + gyro + temp).
 * @param sample  Pointer to receive the data
 * @return 0 on success, -1 on I2C error
 */
int imu_read(imu_sample_t *sample);

/**
 * Read temperature only.
 */
int imu_read_temp(int8_t *temp_c);

#endif /* SPECKLEFLOW_IMU_H */