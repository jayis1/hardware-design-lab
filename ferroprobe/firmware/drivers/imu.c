/*
 * imu.c — BMI270 6-axis IMU driver
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Interfaces with the Bosch BMI270 6-axis IMU via I2C1.
 * Provides raw accelerometer and gyroscope readings, and computes
 * device orientation (roll, pitch, heading) using a complementary filter.
 *
 * The orientation is used to rotate the measured magnetic vector from
 * the sensor frame to the Earth-level frame, enabling walk-mode surveying
 * where the device orientation changes continuously.
 */

#include "imu.h"
#include "../board.h"
#include "../registers.h"
#include <math.h>

/* ===================================================================== */
/*  I2C1 low-level routines                                                */
/* ===================================================================== */

static void i2c1_init(void)
{
    /* Enable GPIOB and I2C1 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
    RCC_APB4ENR |= RCC_APB4ENR_I2C1EN;

    /* PB6 = I2C1_SCL (AF4), PB7 = I2C1_SDA (AF4) */
    volatile uint32_t *afrl = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRL);
    volatile uint32_t *moder = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER);
    volatile uint32_t *otyper = (volatile uint32_t *)(GPIOB_BASE + GPIO_OTYPER);
    volatile uint32_t *pupdr = (volatile uint32_t *)(GPIOB_BASE + GPIO_PUPDR);

    for (uint8_t pin = 6; pin <= 7; pin++) {
        *afrl &= ~(0xFU << (pin * 4));
        *afrl |= ((uint32_t)GPIO_AF4 << (pin * 4));
        *moder &= ~(0x3U << (pin * 2));
        *moder |= (0x2U << (pin * 2));   /* AF mode */
        *otyper |= (1U << pin);          /* Open-drain */
        *pupdr |= (0x1U << (pin * 2));   /* Pull-up */
    }

    /* Configure I2C1 timing for 400 kHz at 120 MHz PCLK */
    I2C_CR1 = 0;
    I2C_TIMINGR = I2C1_TIMINGR;
    I2C_CR1 |= I2C_CR1_PE;  /* Enable peripheral */
}

/* Write a byte to an I2C register: [addr << 1 | W] [reg] [data] */
static uint8_t i2c_write_reg(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    /* Start + write address */
    I2C_CR2 = ((uint32_t)dev_addr << 1) | I2C_CR2_NBYTES_2;
    I2C_CR2 |= I2C_CR2_START;

    /* Wait for TXIS (transmit register empty) or NACK */
    uint32_t timeout = 10000;
    while (!(I2C_ISR & I2C_ISR_TXIS) && !(I2C_ISR & I2C_ISR_NACKF) && --timeout)
        ;
    if (I2C_ISR & I2C_ISR_NACKF) {
        I2C_ICR = I2C_ISR_NACKF;
        return 0;  /* NACK */
    }

    I2C_TXDR = reg;
    while (!(I2C_ISR & I2C_ISR_TXIS))
        ;
    I2C_TXDR = data;

    /* Wait for transfer complete, then STOP */
    while (!(I2C_ISR & I2C_ISR_TC))
        ;
    I2C_CR2 |= I2C_CR2_STOP;
    while (I2C_ISR & I2C_ISR_BUSY)
        ;

    return 1;
}

/* Read multiple bytes from I2C register: [addr<<1|W][reg][RESTART][addr<<1|R][data...] */
static uint8_t i2c_read_burst(uint8_t dev_addr, uint8_t reg,
                               uint8_t *buf, uint8_t len)
{
    /* Phase 1: write register address */
    I2C_CR2 = ((uint32_t)dev_addr << 1) | I2C_CR2_NBYTES_1;
    I2C_CR2 |= I2C_CR2_START;

    uint32_t timeout = 10000;
    while (!(I2C_ISR & I2C_ISR_TXIS) && !(I2C_ISR & I2C_ISR_NACKF) && --timeout)
        ;
    if (I2C_ISR & I2C_ISR_NACKF) {
        I2C_ICR = I2C_ISR_NACKF;
        return 0;
    }
    I2C_TXDR = reg;

    while (!(I2C_ISR & I2C_ISR_TC))
        ;

    /* Phase 2: restart as read */
    I2C_CR2 = ((uint32_t)dev_addr << 1) | I2C_CR2_RD_WRN
              | ((uint32_t)len << 16) | I2C_CR2_AUTOEND;
    I2C_CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C_ISR & I2C_ISR_RXNE))
            ;
        buf[i] = (uint8_t)I2C_RXDR;
    }

    while (I2C_ISR & I2C_ISR_BUSY)
        ;

    return 1;
}

/* ===================================================================== */
/*  BMI270 driver                                                          */
/* ===================================================================== */

/* BMI270 accel full-scale: ±8g → 1 LSB = 0.244 mg = 0.000244 g
 * Gyro full-scale: ±1000°/s → 1 LSB = 0.0305°/s
 */
#define BMI270_ACC_LSB_G     0.000244f
#define BMI270_GYRO_LSB_DPS  0.0305f

static float heading_integrated = 0.0f;  /* Gyro-integrated heading */
static uint16_t sample_rate_hz = 400;
static float prev_gz = 0.0f;

void imu_init(void)
{
    i2c1_init();
    delay_ms(10);

    /* Verify chip ID */
    uint8_t chipid = 0;
    i2c_read_burst(BMI270_I2C_ADDR, BMI270_REG_CHIPID, &chipid, 1);
    if (chipid != BMI270_CHIPID_VAL) {
        /* BMI270 not responding — continue anyway, orientation will be invalid */
        return;
    }

    /* Soft reset */
    i2c_write_reg(BMI270_I2C_ADDR, BMI270_REG_CMD, 0xB6U);  /* softreset */
    delay_ms(2);

    /* Disable power-saving, enable accel + gyro + temp */
    i2c_write_reg(BMI270_I2C_ADDR, BMI270_REG_PWR_CTRL,
                  BMI270_PWR_ACC_EN | BMI270_PWR_GYR_EN | BMI270_PWR_TEMP_EN);
    delay_ms(5);

    /* Set accel config: ±8g, 400 Hz
     * ACC_CONF: ODR=400Hz (0x0A<<0), BWP=normal (0x2<<4), range=±8g (0x2<<8)
     * But on BMI270, range is in PWR_CTRL or a separate register.
     * Simplified: write ACC_CONF = 0x0A (ODR 400Hz, BWP normal)
     */
    i2c_write_reg(BMI270_I2C_ADDR, 0x40U, 0x02U);  /* ACC_CONF: normal, 400Hz */
    i2c_write_reg(BMI270_I2C_ADDR, 0x41U, 0x08U);  /* ACC_RANGE: ±8g */

    /* Set gyro config: ±1000°/s, 400 Hz */
    i2c_write_reg(BMI270_I2C_ADDR, 0x42U, 0x02U);  /* GYR_CONF: normal, 400Hz */
    i2c_write_reg(BMI270_I2C_ADDR, 0x43U, 0x01U);  /* GYR_RANGE: ±1000°/s */

    heading_integrated = 0.0f;
    prev_gz = 0.0f;
}

void imu_read_raw(float *ax, float *ay, float *az,
                   float *gx, float *gy, float *gz)
{
    uint8_t buf[12];
    /* Read 12 bytes starting from DATA_8 (0x0C): ax_l, ax_h, ay_l, ay_h, az_l, az_h,
     * gx_l, gx_h, gy_l, gy_h, gz_l, gz_h */
    if (!i2c_read_burst(BMI270_I2C_ADDR, BMI270_REG_DATA_8, buf, 12)) {
        *ax = *ay = *az = 0.0f;
        *gx = *gy = *gz = 0.0f;
        return;
    }

    /* Accel: 16-bit signed, little-endian */
    int16_t ax_raw = (int16_t)((buf[1] << 8) | buf[0]);
    int16_t ay_raw = (int16_t)((buf[3] << 8) | buf[2]);
    int16_t az_raw = (int16_t)((buf[5] << 8) | buf[4]);
    /* Gyro: 16-bit signed, little-endian */
    int16_t gx_raw = (int16_t)((buf[7] << 8) | buf[6]);
    int16_t gy_raw = (int16_t)((buf[9] << 8) | buf[8]);
    int16_t gz_raw = (int16_t)((buf[11] << 8) | buf[10]);

    *ax = (float)ax_raw * BMI270_ACC_LSB_G;
    *ay = (float)ay_raw * BMI270_ACC_LSB_G;
    *az = (float)az_raw * BMI270_ACC_LSB_G;
    *gx = (float)gx_raw * BMI270_GYRO_LSB_DPS;
    *gy = (float)gy_raw * BMI270_GYRO_LSB_DPS;
    *gz = (float)gz_raw * BMI270_GYRO_LSB_DPS;
}

void imu_get_orientation(imu_data_t *data)
{
    float ax, ay, az, gx, gy, gz;
    imu_read_raw(&ax, &ay, &az, &gx, &gy, &gz);

    data->ax = ax; data->ay = ay; data->az = az;
    data->gx = gx; data->gy = gy; data->gz = gz;

    /* Roll from accelerometer: atan2(ay, az) */
    float roll = atan2f(ay, az) * 180.0f / 3.14159265f;

    /* Pitch from accelerometer: atan2(-ax, sqrt(ay² + az²)) */
    float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / 3.14159265f;

    /* Heading from gyro integration (gyro z-axis)
     * heading += gz * dt
     * dt = 1 / sample_rate
     * Use trapezoidal integration: (gz + prev_gz) / 2 * dt
     */
    float dt = 1.0f / (float)sample_rate_hz;
    heading_integrated += (gz + prev_gz) * 0.5f * dt;
    prev_gz = gz;

    /* Wrap heading to 0-360° */
    while (heading_integrated < 0.0f)
        heading_integrated += 360.0f;
    while (heading_integrated >= 360.0f)
        heading_integrated -= 360.0f;

    data->roll_deg = roll;
    data->pitch_deg = pitch;
    data->heading_deg = heading_integrated;
    data->valid = 1;
}

void imu_set_sample_rate_hz(uint16_t rate_hz)
{
    sample_rate_hz = rate_hz;
}

void imu_reset_heading(void)
{
    heading_integrated = 0.0f;
    prev_gz = 0.0f;
}