/*
 * bme280.c - Bosch BME280 T/RH/P sensor driver (I2C1)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "bme280.h"
#include "board.h"
#include "registers.h"

/* BME280 registers */
#define BME280_ID            0xD0
#define BME280_CTRL_HUM      0xF2
#define BME280_STATUS        0xF3
#define BME280_CTRL_MEAS     0xF4
#define BME280_CONFIG        0xF5
#define BME280_RESET         0xE0
#define BME280_DATA          0xF7

/* Calibration registers */
#define BME280_DIG_T1 0x88
#define BME280_DIG_H1 0xA1

static int32_t  dig_T1, dig_T2, dig_T3;
static int32_t  dig_P1, dig_P2, dig_P3, dig_P4, dig_P5;
static int32_t  dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  dig_H1, dig_H2, dig_H3, dig_H4, dig_H5, dig_H6;
static int32_t  t_fine;

static int i2c_write(uint8_t reg, uint8_t val)
{
    I2C_CR2(I2C1_BASE) = BME280_I2C_ADDR | (2U << 16) | BIT(13);
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C1_BASE) = reg;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) { }
    I2C_TXDR(I2C1_BASE) = val;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C1_BASE) = I2C_ISR_STOPF;
    return 0;
}

static int i2c_read(uint8_t reg, uint8_t *buf, uint8_t len)
{
    /* write reg addr */
    I2C_CR2(I2C1_BASE) = BME280_I2C_ADDR | (1U << 16) | BIT(10);
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C1_BASE) = reg;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC)) { }
    /* restart read */
    I2C_CR2(I2C1_BASE) = BME280_I2C_ADDR | ((uint32_t)len << 16) | BIT(13) | BIT(10);
    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C_RXDR(I2C1_BASE);
    }
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C1_BASE) = I2C_ISR_STOPF;
    return 0;
}

static int32_t concat16(uint8_t lsb, uint8_t msb, int sign)
{
    int32_t v = (int32_t)((uint16_t)msb << 8) | lsb;
    if (sign && v > 32767) v -= 65536;
    return v;
}

int bme280_init(void)
{
    /* I2C1 timing for 400 kHz, CR1 enable */
    I2C_TIMINGR(I2C1_BASE) = 0x10909CEC;
    I2C_CR1(I2C1_BASE) = 1U;

    uint8_t id;
    if (i2c_read(BME280_ID, &id, 1) != 0 || id != 0x60) return -1;

    /* Read calibration params (T/P: 0x88..0xA7, H: 0xA1, 0xE1..0xE7) */
    uint8_t cal[24];
    if (i2c_read(BME280_DIG_T1, cal, 24) != 0) return -1;
    dig_T1 = concat16(cal[0],  cal[1],  0);
    dig_T2 = concat16(cal[2],  cal[3],  1);
    dig_T3 = concat16(cal[4],  cal[5],  1);
    dig_P1 = concat16(cal[6],  cal[7],  0);
    dig_P2 = concat16(cal[8],  cal[9],  1);
    dig_P3 = concat16(cal[10], cal[11], 1);
    dig_P4 = concat16(cal[12], cal[13], 1);
    dig_P5 = concat16(cal[14], cal[15], 1);
    dig_P6 = concat16(cal[16], cal[17], 1);
    dig_P7 = concat16(cal[18], cal[19], 1);
    dig_P8 = concat16(cal[20], cal[21], 1);
    dig_P9 = concat16(cal[22], cal[23], 1);

    uint8_t h1;  i2c_read(BME280_DIG_H1, &h1, 1);
    dig_H1 = h1;
    uint8_t hc[7]; i2c_read(0xE1, hc, 7);
    dig_H2 = concat16(hc[0], hc[1], 1);
    dig_H3 = hc[2];
    dig_H4 = ((int32_t)hc[3] << 4) | (hc[4] & 0x0F);
    dig_H5 = ((int32_t)hc[5] << 4) | (hc[4] >> 4);
    dig_H6 = (int8_t)hc[6];

    /* Humidity x1, Temp/Press x1, normal mode, 500 ms standby */
    i2c_write(BME280_CTRL_HUM,  0x01);
    i2c_write(BME280_CONFIG,    0xA0);
    i2c_write(BME280_CTRL_MEAS, 0x27);
    return 0;
}

static int32_t compensate_t(int32_t adc_t)
{
    int32_t x1 = (adc_t >> 3) - (dig_T1 << 1);
    x1 = (x1 * dig_T2) >> 11;
    int32_t x2 = ((adc_t >> 4) - dig_T1);
    x2 = ((x2 * x2) >> 12) * dig_T3;
    x2 = x2 >> 14;
    t_fine = x1 + x2;
    return (t_fine * 5 + 128) >> 8;
}

static uint32_t compensate_p(int32_t adc_p)
{
    int64_t x1 = ((int64_t)t_fine) - 128000;
    int64_t x2 = x1 * x1 * dig_P6;
    x2 = x2 + ((x1 * dig_P5) << 17);
    x2 = x2 + (((int64_t)dig_P4) << 35);
    x1 = ((x1 * x1 * dig_P3) >> 8) + ((x1 * dig_P2) << 12);
    x1 = (((((int64_t)1) << 47) + x1)) * (dig_P1) >> 33;
    if (x1 == 0) return 0;
    int64_t p = 1048576 - adc_p;
    p = (((p << 31) - x2) * 3125) / x1;
    x1 = ((int64_t)dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    x2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + x1 + x2 + (((int64_t)dig_P7) << 7)) >> 8) * 25;
    return (uint32_t)p / 100;   /* Pa */
}

static uint32_t compensate_h(int32_t adc_h)
{
    int32_t v = t_fine - 76800;
    int32_t a = ((adc_h << 14) - (dig_H4 << 20) - (dig_H5 * v)) + 16384;
    a = a >> 15;
    int32_t b = ((v * dig_H6) >> 10) * (((v * dig_H3) >> 11) + 32768);
    b = (b >> 10) + 2097152;
    b = (b * dig_H2) + 8192;
    v = (a * b) >> 14;
    v = v - (((((v >> 15) * (v >> 15)) >> 7) * dig_H1) >> 4);
    if (v < 0) v = 0;
    if (v > 419430400) v = 419430400;
    return (uint32_t)(v >> 12) / 1024;   /* %RH */
}

int bme280_read(bme280_data_t *out)
{
    uint8_t raw[8];
    if (i2c_read(BME280_DATA, raw, 8) != 0) return -1;
    int32_t adc_p = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_t = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_h = ((int32_t)raw[6] << 8) | raw[7];

    out->temp_c  = (float)compensate_t(adc_t) / 100.0f;
    out->pres_pa = (float)compensate_p(adc_p);
    out->rh_pct  = (float)compensate_h(adc_h);
    return 0;
}