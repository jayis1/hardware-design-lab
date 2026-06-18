/*
 * bme280.c — BME280 environmental sensor driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the Bosch BME280 temperature/humidity/pressure sensor over I2C.
 * Provides fully compensated readings using the on-chip calibration NVM.
 * Used in FloraPulse to correlate plant stress signals with environmental
 * conditions (VPD = vapor pressure deficit, drought stress indicator).
 */

#include "bme280.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ===================================================================== */
/*  Calibration coefficients (read from sensor NVM at init)               */
/* ===================================================================== */

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t cal;
static int32_t t_fine = 0;  /* Carry-over for P/H compensation */

/* ===================================================================== */
/*  I2C1 low-level routines                                              */
/* ===================================================================== */

static void i2c_wait_flag(uint32_t flag)
{
    uint32_t timeout = 100000;
    while (!(I2C_ISR & flag) && timeout--)
        ;
}

static void i2c_wait_idle(void)
{
    uint32_t timeout = 100000;
    while ((I2C_ISR & I2C_ISR_BUSY) && timeout--)
        ;
}

static uint8_t i2c_write(uint8_t addr, const uint8_t *data, uint8_t len)
{
    I2C_CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << 16) | I2C_CR2_AUTOEND;
    I2C_CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        i2c_wait_flag(I2C_ISR_TXIS);
        if (I2C_ISR & I2C_ISR_NACKF) return 0;
        I2C_TXDR = data[i];
    }

    i2c_wait_flag(I2C_ISR_STOPF);
    I2C_ICR = I2C_ISR_STOPF;  /* Clear STOP flag */
    return 1;
}

static uint8_t i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    /* Write register address */
    I2C_CR2 = ((uint32_t)addr << 1) | (1U << 16) | I2C_CR2_RELOAD;
    I2C_CR2 |= I2C_CR2_START;
    i2c_wait_flag(I2C_ISR_TXIS);
    if (I2C_ISR & I2C_ISR_NACKF) {
        I2C_ICR = I2C_ISR_NACKF;
        return 0;
    }
    I2C_TXDR = reg;
    i2c_wait_flag(I2C_ISR_TCR);

    /* Repeated start for read */
    I2C_CR2 = ((uint32_t)addr << 1) | ((uint32_t)len << 16) |
              I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C_CR2 |= I2C_CR2_START;

    for (uint8_t i = 0; i < len; i++) {
        i2c_wait_flag(I2C_ISR_RXNE);
        buf[i] = (uint8_t)I2C_RXDR;
    }

    i2c_wait_flag(I2C_ISR_STOPF);
    I2C_ICR = I2C_ISR_STOPF;
    return 1;
}

/* ===================================================================== */
/*  Initialization                                                       */
/* ===================================================================== */

void bme280_init(void)
{
    /* Enable GPIOB and I2C1 clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    /* PB8 (SCL), PB9 (SDA) as AF4, open-drain, pull-up */
    volatile uint32_t *pb_moder  = (volatile uint32_t *)(GPIOB_BASE + GPIO_MODER);
    volatile uint32_t *pb_otyper = (volatile uint32_t *)(GPIOB_BASE + GPIO_OTYPER);
    volatile uint32_t *pb_pupdr  = (volatile uint32_t *)(GPIOB_BASE + GPIO_PUPDR);
    volatile uint32_t *pb_afrh   = (volatile uint32_t *)(GPIOB_BASE + GPIO_AFRH);

    for (uint8_t pin = 8; pin <= 9; pin++) {
        *pb_moder &= ~(0x3U << (pin * 2));
        *pb_moder |= (0x2U << (pin * 2));     /* AF mode */
        *pb_otyper |= (1U << pin);            /* Open-drain */
        *pb_pupdr |= (0x1U << (pin * 2));     /* Pull-up */
    }
    *pb_afrh &= ~(0xFFU << 0);   /* Clear AF8, AF9 */
    *pb_afrh |= ((uint32_t)GPIO_AF4 << 0) | ((uint32_t)GPIO_AF4 << 4);

    /* Configure I2C1 timing for 100 kHz at 80 MHz PCLK */
    I2C_CR1 = 0;
    I2C_TIMINGR = I2C1_TIMINGR_VAL;
    I2C_CR1 = I2C_CR1_PE;

    /* Read calibration coefficients */
    uint8_t cal_buf[26];
    uint8_t h1_h7[7];

    i2c_read(BME280_I2C_ADDR, BME280_REG_CALIB00, cal_buf, 26);

    /* Parse temperature/pressure calibration */
    cal.dig_T1 = (uint16_t)((cal_buf[1] << 8) | cal_buf[0]);
    cal.dig_T2 = (int16_t)((cal_buf[3] << 8) | cal_buf[2]);
    cal.dig_T3 = (int16_t)((cal_buf[5] << 8) | cal_buf[4]);
    cal.dig_P1 = (uint16_t)((cal_buf[7] << 8) | cal_buf[6]);
    cal.dig_P2 = (int16_t)((cal_buf[9] << 8) | cal_buf[8]);
    cal.dig_P3 = (int16_t)((cal_buf[11] << 8) | cal_buf[10]);
    cal.dig_P4 = (int16_t)((cal_buf[13] << 8) | cal_buf[12]);
    cal.dig_P5 = (int16_t)((cal_buf[15] << 8) | cal_buf[14]);
    cal.dig_P6 = (int16_t)((cal_buf[17] << 8) | cal_buf[16]);
    cal.dig_P7 = (int16_t)((cal_buf[19] << 8) | cal_buf[18]);
    cal.dig_P8 = (int16_t)((cal_buf[21] << 8) | cal_buf[20]);
    cal.dig_P9 = (int16_t)((cal_buf[23] << 8) | cal_buf[22]);
    cal.dig_H1 = cal_buf[25];

    /* Read H2-H6 from 0xE1 */
    i2c_read(BME280_I2C_ADDR, 0xE1U, h1_h7, 7);
    cal.dig_H2 = (int16_t)((h1_h7[1] << 8) | h1_h7[0]);
    cal.dig_H3 = h1_h7[2];
    cal.dig_H4 = (int16_t)(((int16_t)((int8_t)h1_h7[3] << 4) | (h1_h7[4] & 0x0FU)));
    cal.dig_H5 = (int16_t)(((h1_h7[5] & 0xF0U) >> 4) | ((int16_t)h1_h7[6] << 4));
    cal.dig_H6 = (int8_t)h1_h7[6];  /* Simplified; actual H6 is at 0xE7 */

    /* Read H6 separately (0xE7) */
    uint8_t h6;
    i2c_read(BME280_I2C_ADDR, 0xE7U, &h6, 1);
    cal.dig_H6 = (int8_t)h6;

    /* Configure sensor: forced mode, oversampling ×1 for all, filter off */
    uint8_t ctrl_hum = 0x01U;   /* Humidity oversampling ×1 */
    uint8_t ctrl_meas = 0x24U;  /* Temperature ×1, Pressure ×1, Forced mode */
    uint8_t config = 0x00U;    /* Standby 0.5 ms, filter off */

    i2c_write(BME280_I2C_ADDR, &ctrl_hum, 1);
    /* Actually write register address + value */
    uint8_t buf[2];
    buf[0] = BME280_REG_CTRL_HUM; buf[1] = 0x01;
    i2c_write(BME280_I2C_ADDR, buf, 2);
    buf[0] = BME280_REG_CONFIG; buf[1] = 0x00;
    i2c_write(BME280_I2C_ADDR, buf, 2);
    buf[0] = BME280_REG_CTRL_MEAS; buf[1] = 0x24;
    i2c_write(BME280_I2C_ADDR, buf, 2);
}

/* ===================================================================== */
/*  Compensation (from BME280 datasheet)                                 */
/* ===================================================================== */

static int32_t compensate_T(int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) - ((int32_t)cal.dig_T1 << 1))) *
            ((int32_t)cal.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)cal.dig_T1)) *
              ((adc_T >> 4) - ((int32_t)cal.dig_T1))) >> 12) *
            ((int32_t)cal.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t compensate_P(int32_t adc_P)
{
    int32_t var1, var2;
    uint32_t p;
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)cal.dig_P6);
    var2 = var2 + ((var1 * ((int32_t)cal.dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)cal.dig_P4) << 16);
    var1 = (((cal.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((int32_t)cal.dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)cal.dig_P1)) >> 15);
    if (var1 == 0) return 0;
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12)) * 3125U);
    if (p < 0x80000000U) p = (p << 1) / ((uint32_t)var1);
    else p = (p / (uint32_t)var1) << 1;
    var1 = (((int32_t)cal.dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t)cal.dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + cal.dig_P7) >> 4));
    return p;
}

static uint32_t compensate_H(int32_t adc_H)
{
    int32_t v_x1;
    v_x1 = (t_fine - ((int32_t)76800));
    v_x1 = (((((adc_H << 14) - (((int32_t)cal.dig_H4) << 20) -
               (((int32_t)cal.dig_H5) * v_x1)) + ((int32_t)16384)) >> 15) *
            (((((((v_x1 * ((int32_t)cal.dig_H6)) >> 10) *
                 (((v_x1 * ((int32_t)cal.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
               ((int32_t)2097152)) * ((int32_t)cal.dig_H2) + 8192) >> 14));
    v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) *
                      ((int32_t)cal.dig_H1)) >> 4));
    if (v_x1 < 0) v_x1 = 0;
    if (v_x1 > 419430400) v_x1 = 419430400;
    return (uint32_t)(v_x1 >> 12);
}

/* ===================================================================== */
/*  Public API                                                           */
/* ===================================================================== */

void bme280_read(bme280_data_t *data)
{
    uint8_t raw[8];

    /* Trigger forced mode */
    uint8_t buf[2] = { BME280_REG_CTRL_MEAS, 0x25 };
    i2c_write(BME280_I2C_ADDR, buf, 2);

    /* Wait for conversion (max 9.3 ms for ×1 oversampling) */
    for (volatile int i = 0; i < 200000; i++)
        ;

    i2c_read(BME280_I2C_ADDR, BME280_REG_DATA, raw, 8);

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

    int32_t T = compensate_T(adc_T);
    uint32_t P = compensate_P(adc_P);
    uint32_t H = compensate_H(adc_H);

    data->temperature = (float)T / 100.0f;
    data->pressure    = (float)P;
    data->humidity    = (float)H / 1024.0f;
}

float bme280_read_temperature(void)
{
    uint8_t raw[3];
    i2c_read(BME280_I2C_ADDR, BME280_REG_DATA + 3, raw, 3);
    int32_t adc_T = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t T = compensate_T(adc_T);
    return (float)T / 100.0f;
}