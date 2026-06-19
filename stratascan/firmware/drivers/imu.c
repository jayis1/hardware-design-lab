/*
 * imu.c — BMI270 IMU Driver for Dead-Reckoning Position
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements an I2C driver for the Bosch BMI270 6-axis IMU.  The IMU provides
 * accelerometer and gyroscope data used for dead-reckoning position
 * estimation between GNSS fixes.  When the survey wheel provides the primary
 * along-track distance, the IMU provides cross-track drift compensation and
 * orientation (tilt) for 3D positioning.
 *
 * The BMI270 communicates over I2C1 at 400 kHz.  Register reads/writes use
 * a simple I2C polling protocol.
 */

#include "imu.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  BMI270 register addresses                                             */
/* ===================================================================== */

#define BMI270_REG_CHIP_ID    0x00
#define BMI270_REG_ACC_X_LSB  0x0C
#define BMI270_REG_ACC_X_MSB  0x0D
#define BMI270_REG_ACC_Y_LSB  0x0E
#define BMI270_REG_ACC_Y_MSB  0x0F
#define BMI270_REG_ACC_Z_LSB  0x10
#define BMI270_REG_ACC_Z_MSB  0x11
#define BMI270_REG_GYR_X_LSB  0x12
#define BMI270_REG_GYR_X_MSB  0x13
#define BMI270_REG_GYR_Y_LSB  0x14
#define BMI270_REG_GYR_Y_MSB  0x15
#define BMI270_REG_GYR_Z_LSB  0x16
#define BMI270_REG_GYR_Z_MSB  0x17
#define BMI270_REG_CMD        0x7E
#define BMI270_REG_PWR_CTRL   0x7D
#define BMI270_REG_ACC_CONF   0x40
#define BMI270_REG_GYR_CONF   0x42

#define BMI270_CHIP_ID        0x24

/* Sensitivity: ±2g range → 16384 LSB/g; ±125 dps → 262.1 LSB/dps */
#define BMI270_ACC_SENSITIVITY  16384.0f   /* LSB per g */
#define BMI270_GYR_SENSITIVITY  262.1f      /* LSB per dps at ±125 range */
#define GRAVITY_MPS2            9.80665f

/* ===================================================================== */
/*  I2C low-level routines                                                */
/* ===================================================================== */

static void i2c_delay(void)
{
    for (volatile int d = 0; d < 10; d++) { __asm volatile("nop"); }
}

static int i2c_wait_flag(uint32_t mask)
{
    uint32_t timeout = 10000;
    while ((I2C1->ISR & mask) == 0 && timeout) {
        timeout--;
        i2c_delay();
    }
    return (timeout > 0) ? 0 : -1;
}

static int i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t val)
{
    /* Generate START + address + W */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) | (1U << 16) | (0U << 10);  /* 1 byte, write */
    I2C1->CR2 |= I2C_CR2_START;

    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = reg;

    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = val;

    if (i2c_wait_flag(I2C_ISR_TC)) return -1;
    I2C1->CR2 |= I2C_CR2_STOP;

    return 0;
}

static int i2c_read_burst(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    /* Write the register address first */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) | (1U << 16) | (0U << 10);
    I2C1->CR2 |= I2C_CR2_START;
    if (i2c_wait_flag(I2C_ISR_TXIS)) return -1;
    I2C1->TXDR = reg;
    if (i2c_wait_flag(I2C_ISR_TC)) return -1;

    /* Repeated START for read */
    I2C1->CR2 = ((uint32_t)dev_addr << 1) | ((uint32_t)len << 16) | I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        if (i2c_wait_flag(I2C_ISR_RXNE)) return -1;
        buf[i] = (uint8_t)I2C1->RXDR;
    }

    if (i2c_wait_flag(I2C_ISR_TC)) return -1;
    I2C1->CR2 |= I2C_CR2_STOP;

    return 0;
}

/* ===================================================================== */
/*  Position integration state                                            */
/* ===================================================================== */

static float pos_x = 0.0f;
static float pos_y = 0.0f;
static float velocity_x = 0.0f;
static float velocity_y = 0.0f;
static float heading = 0.0f;   /* radians, from gyro integration */
static uint32_t last_update_ms = 0;

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int imu_init(void)
{
    /* Enable I2C1 peripheral */
    I2C1->CR1 = 0;  /* disable before config */
    /* Configure timing for 400 kHz: TIMINGR computed from 120 MHz PCLK
     * For 400 kHz I2C: PRESC=3, SCLL=12, SCLH=9, SDADEL=0, SCLDEL=0
     */
    I2C1->TIMINGR = (3U << 28) | (0U << 20) | (12U << 16) | (0U << 8) | (9U << 0);
    I2C1->CR1 = I2C_CR1_PE;  /* enable */

    /* Read chip ID to verify communication */
    uint8_t chip_id = 0;
    if (i2c_read_burst(IMU_I2C_ADDR, BMI270_REG_CHIP_ID, &chip_id, 1))
        return -1;
    if (chip_id != BMI270_CHIP_ID)
        return -2;

    /* Soft reset */
    i2c_write_reg(IMU_I2C_ADDR, BMI270_REG_CMD, 0xB6);
    board_delay_ms(50);

    /* Enable accelerometer + gyroscope */
    i2c_write_reg(IMU_I2C_ADDR, BMI270_REG_PWR_CTRL, 0x0E);  /* ACC + GYR on */
    board_delay_ms(10);

    /* Configure accelerometer: ±2g, 100 Hz ODR */
    i2c_write_reg(IMU_I2C_ADDR, BMI270_REG_ACC_CONF, 0x28);
    /* Configure gyroscope: ±125 dps, 100 Hz ODR */
    i2c_write_reg(IMU_I2C_ADDR, BMI270_REG_GYR_CONF, 0x28);

    board_delay_ms(10);
    last_update_ms = board_get_tick_ms();
    return 0;
}

void imu_read_raw(int16_t *accel, int16_t *gyro)
{
    uint8_t buf[12];
    /* Read 12 bytes: 6 accel + 6 gyro (little-endian) */
    if (i2c_read_burst(IMU_I2C_ADDR, BMI270_REG_ACC_X_LSB, buf, 12) == 0) {
        accel[0] = (int16_t)((buf[1] << 8) | buf[0]);   /* X */
        accel[1] = (int16_t)((buf[3] << 8) | buf[2]);   /* Y */
        accel[2] = (int16_t)((buf[5] << 8) | buf[4]);   /* Z */
        gyro[0]  = (int16_t)((buf[7] << 8) | buf[6]);   /* X */
        gyro[1]  = (int16_t)((buf[9] << 8) | buf[8]);   /* Y */
        gyro[2]  = (int16_t)((buf[11] << 8) | buf[10]); /* Z */
    } else {
        for (int i = 0; i < 3; i++) { accel[i] = 0; gyro[i] = 0; }
    }
}

void imu_get_delta(float *dx, float *dy)
{
    int16_t accel[3], gyro[3];
    imu_read_raw(accel, gyro);

    /* Convert to physical units */
    float ax = (float)accel[0] / BMI270_ACC_SENSITIVITY * GRAVITY_MPS2;
    float ay = (float)accel[1] / BMI270_ACC_SENSITIVITY * GRAVITY_MPS2;
    float gz = (float)gyro[2] / BMI270_GYR_SENSITIVITY;  /* deg/s */

    /* Time delta */
    uint32_t now = board_get_tick_ms();
    float dt = (float)(now - last_update_ms) / 1000.0f;
    last_update_ms = now;
    if (dt <= 0 || dt > 1.0f) dt = 0.01f;  /* clamp */

    /* Integrate gyro to update heading (Z-axis rotation) */
    heading += gz * dt * (float)(M_PI / 180.0);

    /* Simple dead-reckoning: project horizontal acceleration onto heading */
    float a_forward = ax * cosf(heading) + ay * sinf(heading);
    velocity_x += a_forward * dt;
    /* Clamp velocity to reasonable walking speeds */
    if (velocity_x > 1.5f) velocity_x = 1.5f;
    if (velocity_x < -1.5f) velocity_x = -1.5f;

    /* Integrate position */
    pos_x += velocity_x * cosf(heading) * dt;
    pos_y += velocity_x * sinf(heading) * dt;

    *dx = pos_x;
    *dy = pos_y;
}

void imu_reset_position(void)
{
    pos_x = 0.0f;
    pos_y = 0.0f;
    velocity_x = 0.0f;
    velocity_y = 0.0f;
    heading = 0.0f;
    last_update_ms = board_get_tick_ms();
}

/* End of imu.c */