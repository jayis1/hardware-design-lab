/*
 * imu.h — BMI270 6-axis IMU driver interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_IMU_H
#define FERROPROBE_IMU_H

#include <stdint.h>

/* Orientation data (roll, pitch, heading) */
typedef struct {
    float roll_deg;     /* Rotation around X axis, ±90° */
    float pitch_deg;     /* Rotation around Y axis, ±90° */
    float heading_deg;   /* Rotation around Z axis, 0-360° */
    float ax, ay, az;    /* Raw accelerometer, g */
    float gx, gy, gz;    /* Raw gyroscope, °/s */
    uint32_t timestamp_ms;
    uint8_t valid;
} imu_data_t;

/* Initialize the BMI270 IMU via I2C1.
 * Configures accelerometer (±8g, 400 Hz) and gyroscope (±1000°/s, 400 Hz).
 */
void imu_init(void);

/* Read raw accelerometer and gyroscope data */
void imu_read_raw(float *ax, float *ay, float *az,
                   float *gx, float *gy, float *gz);

/* Compute orientation (roll, pitch, heading) from IMU data.
 * Uses a complementary filter: accelerometer for roll/pitch (low freq),
 * gyroscope integration for heading (high freq).
 */
void imu_get_orientation(imu_data_t *data);

/* Set the gyroscope integration sample rate (for heading calc) */
void imu_set_sample_rate_hz(uint16_t rate_hz);

/* Reset the heading integration (set heading to 0) */
void imu_reset_heading(void);

#endif /* FERROPROBE_IMU_H */