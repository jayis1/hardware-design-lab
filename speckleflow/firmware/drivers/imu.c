/*
 * imu.c — ICM-42688-P 6-axis IMU driver for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The ICM-42688-P provides 3-axis accelerometer and 3-axis gyroscope
 * data over I2C4 at 0x69. We use it to log device orientation for
 * each frame, enabling image registration and stabilization in
 * post-processing.
 */

#include "imu.h"
#include "board.h"
#include "registers.h"
#include <string.h>

#define ICM42688_ADDR  0x69

/* Register map */
#define ICM_REG_WHO_AM_I     0x75  /* should read 0x47 */
#define ICM_REG_DEVICE_CONFIG 0x11
#define ICM_REG_PWR_MGMT0    0x4E
#define ICM_REG_GYRO_CONFIG0  0x4F
#define ICM_REG_ACCEL_CONFIG0 0x50
#define ICM_REG_GYRO_CONFIG1  0x51
#define ICM_REG_ACCEL_CONFIG1 0x53
#define ICM_REG_INT_CONFIG    0x14
#define ICM_REG_INT_SOURCE0   0x65
#define ICM_REG_TEMP_DATA0    0x1D
#define ICM_REG_ACCEL_DATA_X0 0x1F
#define ICM_REG_GYRO_DATA_X0  0x25

static struct {
    int16_t accel[3];
    int16_t gyro[3];
    int16_t temp;
} imu_data;

/* ---- I2C4 primitives --------------------------------------------------- */

static void i2c4_wait_tx(void) {
    while (!(I2C4->ISR & I2C_ISR_TXE)) { }
}

static void i2c4_wait_rx(void) {
    while (!(I2C4->ISR & I2C_ISR_RXNE)) { }
}

static void i2c4_wait_tc(void) {
    while (!(I2C4->ISR & I2C_ISR_TC)) { }
}

static void i2c4_wait_stop(void) {
    while (!(I2C4->ISR & I2C_ISR_STOPF)) { }
    I2C4->ICR = I2C_ISR_STOPF;
}

static int i2c4_write_reg(uint8_t reg, uint8_t val) {
    uint32_t timeout = 100000;
    while (I2C4->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -1;
    }

    I2C4->CR2 = ((uint32_t)ICM42688_ADDR << 1)
              | (2u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_AUTOEND;
    I2C4->CR2 |= I2C_CR2_START;

    i2c4_wait_tx();
    if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
    I2C4->TXDR = reg;
    i2c4_wait_tx();
    if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
    I2C4->TXDR = val;
    i2c4_wait_stop();
    return 0;
}

static int i2c4_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint32_t timeout = 100000;
    while (I2C4->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -1;
    }

    /* Write register address (reload, no stop) */
    I2C4->CR2 = ((uint32_t)ICM42688_ADDR << 1)
              | (1u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_RELOAD;
    I2C4->CR2 |= I2C_CR2_START;
    i2c4_wait_tx();
    if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
    I2C4->TXDR = reg;
    i2c4_wait_tc();

    /* Read len bytes (auto-end) */
    I2C4->CR2 = ((uint32_t)ICM42688_ADDR << 1)
              | I2C_CR2_RD_WRN
              | ((uint32_t)len << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_AUTOEND;
    I2C4->CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        i2c4_wait_rx();
        buf[i] = (uint8_t)(I2C4->RXDR & 0xFF);
    }
    i2c4_wait_stop();
    return 0;
}

/* ---- Public API --------------------------------------------------------- */

int imu_init(void) {
    memset(&imu_data, 0, sizeof(imu_data));

    /* Verify chip ID */
    uint8_t whoami = 0;
    if (i2c4_read_regs(ICM_REG_WHO_AM_I, &whoami, 1) != 0) return -1;
    if (whoami != 0x47) return -2;

    /* Software reset */
    i2c4_write_reg(ICM_REG_DEVICE_CONFIG, 0x01);
    for (volatile int i = 0; i < 100000; i++) { }

    /* Power management: enable gyro + accel, low-noise mode */
    i2c4_write_reg(ICM_REG_PWR_MGMT0, 0x0F);

    /* Gyro: ±2000 dps, ODR = 1 kHz (LN mode) */
    i2c4_write_reg(ICM_REG_GYRO_CONFIG0, 0x06);

    /* Accel: ±16g, ODR = 1 kHz */
    i2c4_write_reg(ICM_REG_ACCEL_CONFIG0, 0x06);

    /* Temp: low-pass filter bandwidth = 50 Hz */
    i2c4_write_reg(ICM_REG_GYRO_CONFIG1, 0x09);
    i2c4_write_reg(ICM_REG_ACCEL_CONFIG1, 0x09);

    return 0;
}

int imu_read(imu_sample_t *sample) {
    uint8_t buf[12];

    /* Read accel (6 bytes) + temp (2 bytes) + gyro (6 bytes) in one burst */
    if (i2c4_read_regs(ICM_REG_ACCEL_DATA_X0, buf, 12) != 0) return -1;

    /* Accel X, Y, Z (big-endian!) */
    imu_data.accel[0] = (int16_t)((buf[0] << 8) | buf[1]);
    imu_data.accel[1] = (int16_t)((buf[2] << 8) | buf[3]);
    imu_data.accel[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* Gyro X, Y, Z (big-endian) */
    imu_data.gyro[0] = (int16_t)((buf[6] << 8) | buf[7]);
    imu_data.gyro[1] = (int16_t)((buf[8] << 8) | buf[9]);
    imu_data.gyro[2] = (int16_t)((buf[10] << 8) | buf[11]);

    /* Read temperature separately */
    uint8_t temp_buf[2];
    if (i2c4_read_regs(ICM_REG_TEMP_DATA0, temp_buf, 2) != 0) return -1;
    imu_data.temp = (int16_t)((temp_buf[0] << 8) | temp_buf[1]);

    /* Copy to output */
    sample->accel_x = imu_data.accel[0];
    sample->accel_y = imu_data.accel[1];
    sample->accel_z = imu_data.accel[2];
    sample->gyro_x  = imu_data.gyro[0];
    sample->gyro_y  = imu_data.gyro[1];
    sample->gyro_z  = imu_data.gyro[2];
    sample->temp_c  = (int8_t)((imu_data.temp / 132) + 25);  /* per datasheet */

    return 0;
}

int imu_read_temp(int8_t *temp_c) {
    uint8_t buf[2];
    if (i2c4_read_regs(ICM_REG_TEMP_DATA0, buf, 2) != 0) return -1;
    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    *temp_c = (int8_t)((raw / 132) + 25);
    return 0;
}