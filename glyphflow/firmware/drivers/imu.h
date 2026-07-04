/*
 * imu.h — ICM-42688-P 6-axis IMU driver (SPI)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_IMU_H
#define GLYPHFLOW_IMU_H

#include <stdint.h>
#include "board.h"

/* A single 6-axis sample, axes in sensor LSB (int16). */
typedef struct {
    int16_t ax, ay, az;   /* accel LSB, ±8 g, 4096 LSB/g */
    int16_t gx, gy, gz;   /* gyro LSB, ±2000 °/s, 16.4 LSB/(°/s) */
} imu_sample_t;

/* Wake-on-motion configuration. */
typedef struct {
    uint16_t threshold_mg;   /* motion threshold in milli-g (1–~1000) */
    uint8_t  duration_ms;     /* minimum motion duration, ms (1–~250) */
} imu_wom_config_t;

int      imu_init(void);
int      imu_configure_1khz(void);
int      imu_configure_wom(const imu_wom_config_t *cfg);
int      imu_read_fifo(imu_sample_t *out, uint8_t max_count, uint8_t *read_count);
int      imu_read_register(uint8_t reg, uint8_t *val);
int      imu_write_register(uint8_t reg, uint8_t val);
int      imu_clear_int1(void);
int      imu_disable_fifo(void);
int      imu_enable_fifo(void);
uint8_t  imu_whoami(void);

#endif /* GLYPHFLOW_IMU_H */