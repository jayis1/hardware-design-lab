/*
 * imu.h — ICM-42688-P 6-channel IMU driver header.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_IMU_H
#define SYNTHAND_IMU_H

#include <stdint.h>
#include "board.h"

/* IMU data sample — one per chip per 2 ms tick */
typedef struct {
    int16_t accel[3];   /* X, Y, Z in raw LSB (±16g range, 16-bit) */
    int16_t gyro[3];    /* X, Y, Z in raw LSB (±2000 dps, 16-bit) */
    int16_t temp;       /* temperature raw */
    uint32_t timestamp; /* sample timestamp (ms) */
} imu_sample_t;

/* Initialize SPI0 bus and all 6 ICM-42688-P chips.
 * Returns 0 on success, nonzero on error. */
int imu_init(void);

/* Enable/disable IMU sampling (power gate the chips).
 * When enabled, chips are in low-noise mode at 500 Hz ODR.
 * When disabled, chips enter low-power mode (6 µA each). */
void imu_enable(int enable);

/* Read all 6 IMUs via round-robin SPI.
 * Fills the array of 6 imu_sample_t.
 * Returns 0 on success, nonzero on SPI error. */
int imu_read_all(imu_sample_t *samples);

/* Perform gyroscope bias calibration (requires static hand).
 * Averages 512 samples (~1 second) and stores bias in calibration_t. */
int imu_calibrate_gyro(int16_t bias_out[NUM_IMUS][3]);

/* Perform accelerometer bias calibration.
 * Requires hand flat, palm down. Averages 512 samples. */
int imu_calibrate_accel(int16_t bias_out[NUM_IMUS][3]);

/* ICM-42688-P register addresses */
#define ICM_REG_WHO_AM_I     0x75  /* should return 0x47 */
#define ICM_REG_PWR_MGMT0    0x4E
#define ICM_REG_GYRO_CONFIG0 0x4F
#define ICM_REG_ACCEL_CONFIG0 0x50
#define ICM_REG_CONFIG0      0x53
#define ICM_REG_INT_CONFIG   0x14
#define ICM_REG_ACCEL_DATA_X1 0x1F
#define ICM_REG_GYRO_DATA_X1  0x25
#define ICM_REG_TEMP_DATA1   0x2B

/* PWR_MGMT0 values */
#define ICM_PWR_LOW_NOISE    0x0F  /* gyro + accel, low-noise mode */
#define ICM_PWR_LOW_POWER    0x0B  /* gyro + accel, low-power */
#define ICM_PWR_OFF          0x00  /* everything off */

/* Config values for 500 Hz ODR, ±16g, ±2000 dps */
#define ICM_ACCEL_CONFIG_16G_500HZ  0x08  /* FS=±16g, ODR=500Hz */
#define ICM_GYRO_CONFIG_2000DPS_500HZ 0x08 /* FS=±2000dps, ODR=500Hz */

#endif /* SYNTHAND_IMU_H */