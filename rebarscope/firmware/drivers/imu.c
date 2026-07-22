/*
 * drivers/imu.c — LSM6DSO 6-axis IMU driver (I2C1)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "../board.h"
#include "../registers.h"
#include "imu.h"

#define LSM6DSO_WHO_AM_I      0x0F
#define LSM6DSO_CTRL1_XL      0x10
#define LSM6DSO_CTRL2_G        0x11
#define LSM6DSO_STATUS_REG     0x1E
#define LSM6DSO_OUTX_L_G       0x22
#define LSM6DSO_OUTX_L_XL      0x28

static float g_yaw_rate_dps = 0.0f;
static float g_yaw_deg = 0.0f;

static void i2c1_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

static int i2c1_wait_flag(uint32_t mask, int set)
{
    uint32_t timeout = 100000u;
    while (set ? !(I2C1->ISR & mask) : (I2C1->ISR & mask)) {
        if (--timeout == 0) return -1;
        i2c1_delay_us(1);
    }
    return 0;
}

static int i2c1_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | (1u << 16) | I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_TXIS, 1)) return -1;
    I2C1->TXDR = reg;
    if (i2c1_wait_flag(I2C_ISR_TXIS, 1)) return -1;
    I2C1->TXDR = val;
    if (i2c1_wait_flag(I2C_ISR_TC, 1)) return -1;
    I2C1->CR2 = I2C_CR2_STOP;
    return 0;
}

static int i2c1_read_burst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n)
{
    I2C1->CR2 = ((uint32_t)addr << 1) | (1u << 16) | I2C_CR2_START;
    if (i2c1_wait_flag(I2C_ISR_TXIS, 1)) return -1;
    I2C1->TXDR = reg;
    if (i2c1_wait_flag(I2C_ISR_TC, 1)) return -1;
    /* repeated start for read */
    I2C1->CR2 = ((uint32_t)addr << 1) | (1u << 1) | ((uint32_t)n << 16) | I2C_CR2_START;
    for (uint8_t i = 0; i < n; i++) {
        if (i2c1_wait_flag(I2C_ISR_RXNE, 1)) return -1;
        buf[i] = I2C1->RXDR;
    }
    if (i2c1_wait_flag(I2C_ISR_TC, 1)) return -1;
    I2C1->CR2 = I2C_CR2_STOP;
    return 0;
}

void imu_init(void)
{
    /* Enable I2C1 clock + pins (done in board_init); reset sensor */
    i2c1_write_reg(IMU_ADDR, LSM6DSO_CTRL1_XL, 0x00);
    i2c1_write_reg(IMU_ADDR, LSM6DSO_CTRL2_G,  0x00);
    i2c1_delay_us(500);

    /* Gyro: 104 Hz, 500 dps full-scale → 17.5 mdps/LSB */
    i2c1_write_reg(IMU_ADDR, LSM6DSO_CTRL2_G, 0x64);
    /* Accel: 104 Hz, 2 g full-scale → 0.061 mg/LSB */
    i2c1_write_reg(IMU_ADDR, LSM6DSO_CTRL1_XL, 0x64);
    i2c1_delay_us(100);
    g_yaw_deg = 0.0f;
    g_yaw_rate_dps = 0.0f;
}

float imu_get_yaw_rate_dps(void)
{
    return g_yaw_rate_dps;
}

float imu_get_yaw_deg(void)
{
    return g_yaw_deg;
}

void imu_reset_yaw(void)
{
    g_yaw_deg = 0.0f;
}

/* Called at 50 Hz from encoder_update() */
void imu_poll(void)
{
    uint8_t buf[6];
    if (i2c1_read_burst(IMU_ADDR, LSM6DSO_OUTX_L_G, buf, 6) == 0) {
        int16_t gx = (int16_t)((buf[1] << 8) | buf[0]);
        /* 17.5 mdps/LSB → 0.0175 dps/LSB; full-scale check */
        g_yaw_rate_dps = (float)gx * 0.0175f;
        /* Dead-reckon yaw (degrees) at 20 ms tick */
        g_yaw_deg += g_yaw_rate_dps * 0.02f;
        if (g_yaw_deg >= 360.0f) g_yaw_deg -= 360.0f;
        if (g_yaw_deg < 0.0f)    g_yaw_deg += 360.0f;
    }
}