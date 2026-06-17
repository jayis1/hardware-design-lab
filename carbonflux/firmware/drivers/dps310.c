/**
 * @file    dps310.c
 * @brief   DPS310 barometric pressure sensor driver for CarbonFlux.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * The DPS310 is a high-precision barometric pressure sensor (±0.002 hPa)
 * with on-chip temperature compensation. Used in CarbonFlux to provide
 * the pressure term in the flux equation.
 */

#include "dps310.h"
#include "../board.h"
#include "../registers.h"
#include <stddef.h>

/* ======================================================================== */
/*  REGISTER MAP                                                            */
/* ======================================================================== */

#define DPS310_REG_PSR_B2       0x00    /* Pressure MSB */
#define DPS310_REG_PSR_B1       0x01    /* Pressure middle */
#define DPS310_REG_PSR_B0       0x02    /* Pressure LSB */
#define DPS310_REG_TMP_B2       0x03    /* Temperature MSB */
#define DPS310_REG_TMP_B1       0x04    /* Temperature middle */
#define DPS310_REG_TMP_B0       0x05    /* Temperature LSB */
#define DPS310_REG_PRS_CFG      0x06    /* Pressure config */
#define DPS310_REG_TMP_CFG      0x07    /* Temperature config */
#define DPS310_REG_MEAS_CFG     0x08    /* Measurement config */
#define DPS310_REG_CFG_REG      0x09    /* Configuration register */
#define DPS310_REG_RESET        0x0C    /* Soft reset */
#define DPS310_REG_COEF         0x10    /* Coefficients start (18 bytes) */
#define DPS310_REG_COEF_SRCE    0x28    /* Coefficient source */

/* ======================================================================== */
/*  COMMAND BITS                                                            */
/* ======================================================================== */

/* TMP_CFG */
#define DPS310_TMP_EXT          0x80    /* External temperature sensor */
#define DPS310_TMP_RATE_1       0x00    /* 1 measurement/s */
#define DPS310_TMP_RATE_2       0x10    /* 2 measurements/s */
#define DPS310_TMP_RATE_4       0x20    /* 4 measurements/s */
#define DPS310_TMP_RATE_8       0x30    /* 8 measurements/s */
#define DPS310_TMP_RATE_16      0x40    /* 16 measurements/s */
#define DPS310_TMP_RATE_32      0x50    /* 32 measurements/s */
#define DPS310_TMP_RATE_64      0x60    /* 64 measurements/s */
#define DPS310_TMP_RATE_128     0x70    /* 128 measurements/s */

#define DPS310_TMP_PREC_16      0x00    /* 16x oversampling */
#define DPS310_TMP_PREC_32      0x01    /* 32x oversampling */
#define DPS310_TMP_PREC_64      0x02    /* 64x oversampling */
#define DPS310_TMP_PREC_128     0x03    /* 128x oversampling */

/* PRS_CFG */
#define DPS310_PRS_RATE_1       0x00
#define DPS310_PRS_RATE_2       0x10
#define DPS310_PRS_RATE_4       0x20
#define DPS310_PRS_RATE_8       0x30
#define DPS310_PRS_RATE_16      0x40
#define DPS310_PRS_RATE_32      0x50
#define DPS310_PRS_RATE_64      0x60
#define DPS310_PRS_RATE_128     0x70

#define DPS310_PRS_PREC_16      0x00
#define DPS310_PRS_PREC_32      0x01
#define DPS310_PRS_PREC_64      0x02
#define DPS310_PRS_PREC_128     0x03

/* MEAS_CFG */
#define DPS310_MEAS_CFG_COEF_RDY 0x80    /* Coefficient ready indicator */
#define DPS310_MEAS_CFG_SENSOR_RDY 0x40  /* Sensor ready */
#define DPS310_MEAS_CFG_TMP_RDY   0x20   /* Temperature measurement ready */
#define DPS310_MEAS_CFG_PRS_RDY   0x10   /* Pressure measurement ready */
#define DPS310_MEAS_CFG_CONT_PRS  0x04   /* Continuous pressure mode */
#define DPS310_MEAS_CFG_CONT_TMP  0x02   /* Continuous temperature mode */
#define DPS310_MEAS_CFG_CONT_BOTH 0x07   /* Both continuous */

#define DPS310_MEAS_CFG_CMD_PRS  0x01    /* Single pressure measurement */
#define DPS310_MEAS_CFG_CMD_TMP  0x02    /* Single temperature measurement */

/* CFG_REG */
#define DPS310_CFG_FIFO_EN      0x02     /* Enable FIFO */
#define DPS310_CFG_SPI_MODE     0x01     /* SPI mode (unused on I2C) */

/* RESET */
#define DPS310_RESET_KEY        0x89     /* Soft reset key */

/* ======================================================================== */
/*  I²C HELPER MACROS                                                       */
/* ======================================================================== */

#define DPS310_I2C_ADDR_8BIT    (DPS310_I2C_ADDR << 1)
#define I2C_TIMEOUT             50000

/* ======================================================================== */
/*  LOCAL STATE                                                             */
/* ======================================================================== */

static struct {
    bool    initialized;
    bool    tmp_ext;
    int16_t c0;                 /* Coefficient c0 */
    int16_t c1;                 /* Coefficient c1 */
    int32_t c00;                /* Coefficient c00 */
    int32_t c01;                /* Coefficient c01 */
    int32_t c10;                /* Coefficient c10 */
    int32_t c11;                /* Coefficient c11 */
    int32_t c20;                /* Coefficient c20 */
    int32_t c21;                /* Coefficient c21 */
    int32_t c30;                /* Coefficient c30 */
    float   last_temp_c;        /* Last temperature (°C) */
    float   last_pressure_hpa;  /* Last pressure (hPa) */
} g_dps310 = {0};

/* ======================================================================== */
/*  I²C LOW-LEVEL                                                           */
/* ======================================================================== */

static int dps310_i2c_write_reg(uint8_t reg, uint8_t val) {
    uint32_t timeout = I2C_TIMEOUT;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (DPS310_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (2 << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_START;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = reg;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = val;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

static int dps310_i2c_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint32_t timeout;

    /* Write register address first */
    timeout = I2C_TIMEOUT;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (DPS310_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (1 << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_START;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = reg;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;

    /* Repeated START for read */
    timeout = I2C_TIMEOUT;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--timeout == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (DPS310_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (len << I2C_CR2_NBYTES_Pos)
              | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START;
    for (uint8_t i = 0; i < len; i++) {
        timeout = I2C_TIMEOUT;
        while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if (I2C1->ISR & I2C_ISR_STOPF) break;
            if (--timeout == 0) return -ERR_I2C_TIMEOUT;
        }
        buf[i] = (uint8_t)(I2C1->RXDR & 0xFF);
    }
    timeout = I2C_TIMEOUT;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (--timeout == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

/* ======================================================================== */
/*  COEFFICIENT PARSING                                                     */
/* ======================================================================== */

static int dps310_read_coefficients(void) {
    uint8_t coef[18];
    int ret = dps310_i2c_read_regs(DPS310_REG_COEF, coef, 18);
    if (ret < 0) return ret;

    /* Parse according to DPS310 datasheet */
    g_dps310.c0  = ((int16_t)(((uint16_t)coef[0] << 8) | coef[1])) >> 4;
    g_dps310.c1  = ((int16_t)(((uint16_t)coef[1] & 0x0F) << 8) | coef[2]) << 4; g_dps310.c1 >>= 4;

    g_dps310.c00 = ((int32_t)(((uint32_t)coef[3] << 8) | coef[4]) << 12)
                   | ((uint32_t)coef[5] << 4);
    g_dps310.c00 = (g_dps310.c00 << 12) >> 12;

    g_dps310.c01 = ((int32_t)((uint32_t)coef[6] << 4) | (coef[7] >> 4)) << 18;
    g_dps310.c01 >>= 18;

    g_dps310.c10 = ((int32_t)(((uint32_t)coef[8] << 8) | coef[9]) << 12)
                   | (coef[10] << 4);
    g_dps310.c10 = (g_dps310.c10 << 12) >> 12;

    g_dps310.c11 = ((int32_t)(((uint32_t)coef[11] << 8) | coef[12]) << 12)
                   | (coef[13] << 4);
    g_dps310.c11 = (g_dps310.c11 << 12) >> 12;

    g_dps310.c20 = ((int32_t)(((uint32_t)coef[14] << 8) | coef[15]) << 12)
                   | (coef[16] << 4);
    g_dps310.c20 = (g_dps310.c20 << 12) >> 12;

    g_dps310.c21 = ((int32_t)((uint16_t)coef[17] << 8)) >> 4;

    g_dps310.c30 = 0; /* Not used in standard mode */

    return 0;
}

/* ======================================================================== */
/*  PUBLIC API                                                              */
/* ======================================================================== */

int dps310_init(void) {
    uint8_t id;
    int ret;

    /* Read product ID to verify communication */
    ret = dps310_i2c_read_regs(0x0D, &id, 1);  /* PROD_ID register */
    if (ret < 0) return ret;
    if (id != 0x10) return -ERR_SENSOR_NOT_FOUND;

    /* Soft reset */
    ret = dps310_i2c_write_reg(DPS310_REG_RESET, DPS310_RESET_KEY);
    if (ret < 0) return ret;
    for (volatile uint32_t i = 0; i < 100000; i++);

    /* Read calibration coefficients */
    ret = dps310_read_coefficients();
    if (ret < 0) return ret;

    /* Configure: high-precision, continuous both */
    /* Temperature: 128x oversampling, 128 measurements/s */
    ret = dps310_i2c_write_reg(DPS310_REG_TMP_CFG,
                               DPS310_TMP_RATE_128 | DPS310_TMP_PREC_128);
    if (ret < 0) return ret;

    /* Pressure: 128x oversampling, 128 measurements/s */
    ret = dps310_i2c_write_reg(DPS310_REG_PRS_CFG,
                               DPS310_PRS_RATE_128 | DPS310_PRS_PREC_128);
    if (ret < 0) return ret;

    /* Start continuous pressure + temperature */
    ret = dps310_i2c_write_reg(DPS310_REG_MEAS_CFG, DPS310_MEAS_CFG_CONT_BOTH);
    if (ret < 0) return ret;

    g_dps310.initialized = true;
    return 0;
}

float dps310_read_pressure_hpa(void) {
    if (!g_dps310.initialized) return 0.0f;

    uint8_t buf[6];
    int ret = dps310_i2c_read_regs(DPS310_REG_PSR_B2, buf, 6);
    if (ret < 0) return g_dps310.last_pressure_hpa;  /* Return last valid */

    /* Reconstruct 24-bit pressure and temperature raw values */
    int32_t raw_p = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2];
    if (raw_p & 0x800000) raw_p |= 0xFF000000;  /* Sign-extend */

    int32_t raw_t = ((int32_t)buf[3] << 16) | ((int32_t)buf[4] << 8) | buf[5];
    if (raw_t & 0x800000) raw_t |= 0xFF000000;

    /* Temperature compensation */
    float t_raw_scaled = (float)raw_t / 16777216.0f;
    float temp_c = (float)g_dps310.c0 * 0.5f + (float)g_dps310.c1 * t_raw_scaled;
    g_dps310.last_temp_c = temp_c;

    /* Pressure compensation */
    float p_raw_scaled = (float)raw_p / 16777216.0f;
    float pressure = (float)g_dps310.c00
                   + p_raw_scaled * ((float)g_dps310.c10
                   + p_raw_scaled * ((float)g_dps310.c20
                   + p_raw_scaled * (float)g_dps310.c30))
                   + t_raw_scaled * ((float)g_dps310.c01
                   + p_raw_scaled * ((float)g_dps310.c11));

    /* Convert from Pa to hPa */
    g_dps310.last_pressure_hpa = pressure / 100.0f;
    return g_dps310.last_pressure_hpa;
}

float dps310_read_temp_c(void) {
    if (!g_dps310.initialized) return 0.0f;

    uint8_t buf[3];
    int ret = dps310_i2c_read_regs(DPS310_REG_TMP_B2, buf, 3);
    if (ret < 0) return g_dps310.last_temp_c;

    int32_t raw_t = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | buf[2];
    if (raw_t & 0x800000) raw_t |= 0xFF000000;

    float t_raw_scaled = (float)raw_t / 16777216.0f;
    g_dps310.last_temp_c = (float)g_dps310.c0 * 0.5f + (float)g_dps310.c1 * t_raw_scaled;
    return g_dps310.last_temp_c;
}

int dps310_wake(void) {
    /* DPS310 doesn't have a dedicated sleep/wake mode in continuous mode */
    /* Just verify communication */
    uint8_t id;
    int ret = dps310_i2c_read_regs(0x0D, &id, 1);
    if (ret == 0 && id == 0x10) return 0;
    return dps310_init();
}