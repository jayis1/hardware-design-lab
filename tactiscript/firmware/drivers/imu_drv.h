/*
 * imu_drv.h — TactiScript IMU (ICM-42688-P) driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_IMU_DRV_H
#define TACTISCRIPT_IMU_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Gesture types detected by the IMU driver */
#define IMU_GESTURE_NONE        0
#define IMU_GESTURE_TAP         1
#define IMU_GESTURE_DOUBLE_TAP   2
#define IMU_GESTURE_SWIPE_LEFT  3
#define IMU_GESTURE_SWIPE_RIGHT 4
#define IMU_GESTURE_LONG_PRESS  5
#define IMU_GESTURE_TILT_UP     6
#define IMU_GESTURE_TILT_DOWN   7

/* Initialize the IMU: configure SPI1, set ODR, ranges, FIFO, interrupts */
void imu_init(void);

/* Enter low-power mode (used when device sleeps) */
void imu_enter_low_power(void);

/* Poll for a detected gesture. Returns IMU_GESTURE_NONE if no new gesture.
 * This is non-blocking and reads from a gesture queue filled by the ISR.
 */
int imu_get_gesture(void);

/* Read raw accelerometer data (x, y, z in mg) */
void imu_read_accel(int16_t *x, int16_t *y, int16_t *z);

/* Read raw gyroscope data (x, y, z in deg/s) */
void imu_read_gyro(int16_t *x, int16_t *y, int16_t *z);

/* Read temperature in degrees Celsius (×100, e.g. 2512 = 25.12°C) */
int16_t imu_read_temp(void);

/* Get FIFO sample count */
uint16_t imu_fifo_count(void);

/* Read FIFO data into buffer (returns number of samples read) */
uint16_t imu_fifo_read(int16_t *accel_out, int16_t *gyro_out, uint16_t max);

#endif /* TACTISCRIPT_IMU_DRV_H */