/*
 * imu.h — BMI270 + BMM150 inertial measurement driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_IMU_H
#define INKWELL_DRIVERS_IMU_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float    accel_g[3];    /* x, y, z in g */
    float    gyro_radps[3]; /* x, y, z in rad/s */
    float    mag_ut[3];     /* x, y, z in µT */
    float    dt_s;         /* time since previous sample */
    uint32_t ts_ms;        /* timestamp */
} imu_sample_t;

void    imu_init(void);
int32_t imu_fifo_drain(imu_sample_t *out, uint32_t max_n);
bool    imu_read_mag(float *bx, float *by, float *bz);
void    imu_enable_drdy_irq(void (*cb)(void));
void    imu_reset(void);

#endif