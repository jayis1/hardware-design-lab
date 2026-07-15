/*
 * imu_gesture.h — IMU polling and gesture recognition
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef IMU_GESTURE_H
#define IMU_GESTURE_H

#include <stdint.h>
#include "registers.h"

/* IMU data sample (raw sensor values) */
typedef struct {
    int16_t accel_x, accel_y, accel_z;  /* mg (milli-g) */
    int16_t gyro_x, gyro_y, gyro_z;     /* mdps (milli-degrees/s) */
    int16_t temp_c100;                   /* °C × 100 */
    uint32_t timestamp_ms;               /* sample timestamp */
} imu_sample_t;

/* Initialize IMU (ICM-42688-P via SPI) */
bool imu_init(void);

/* Read one IMU sample (called at 200 Hz from imu_task) */
bool imu_read(imu_sample_t *sample);

/* Gesture detection: call with each new sample.
 * Returns GESTURE_NONE if no gesture detected, or a GESTURE_* code.
 * Internally tracks state for double-tap, flip, shake. */
uint8_t imu_detect_gesture(const imu_sample_t *sample);

/* Reset gesture state machine */
void imu_reset_gesture(void);

/* Get orientation (pitch/roll in degrees, for telemetry) */
void imu_get_orientation(int16_t *pitch_deg, int16_t *roll_deg);

#endif /* IMU_GESTURE_H */