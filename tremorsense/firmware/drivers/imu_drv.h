/*
 * imu_drv.h — ICM-42688-P IMU driver header
 * TremorSense — Wearable Tremor Characterization Band
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */

#ifndef TREMORSENSE_IMU_DRV_H
#define TREMORSENSE_IMU_DRV_H

#include <stdint.h>
#include <stddef.h>

/* IMU data sample: 6-axis, int16 per axis */
typedef struct {
    int16_t accel[3];  /* X, Y, Z in raw LSB */
    int16_t gyro[3];   /* X, Y, Z in raw LSB */
} imu_sample_t;

/* Callback for FIFO watermark interrupt */
typedef void (*imu_irq_callback_t)(void);

/* Initialization */
int  imu_init(void);
void imu_start_sampling(uint16_t odr_hz, uint8_t accel_fs, uint8_t gyro_fs,
                         uint16_t fifo_watermark);
void imu_stop_sampling(void);

/* Register read/write */
uint8_t imu_read_reg(uint8_t reg);
void    imu_write_reg(uint8_t reg, uint8_t val);
void    imu_read_regs(uint8_t reg, uint8_t *buf, uint16_t len);

/* FIFO read: returns bytes read, data placed in buf */
uint16_t imu_read_fifo(uint8_t *buf, uint16_t max_len);

/* Interrupt registration */
void imu_register_irq_callback(imu_irq_callback_t cb);

/* Self-test */
int  imu_self_test(void);

/* Who-am-I check */
uint8_t imu_who_am_i(void);

/* Get current sensitivity factors */
float imu_get_accel_sensitivity(void);
float imu_get_gyro_sensitivity(void);

/* Sleep / wake for power management */
void imu_enter_sleep(void);
void imu_wake_from_sleep(void);

/* Wake-on-motion configuration (ultra-low-power mode) */
void imu_configure_wake_on_motion(uint16_t threshold_mg);

#endif /* TREMORSENSE_IMU_DRV_H */