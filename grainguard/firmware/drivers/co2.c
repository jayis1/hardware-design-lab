/*
 * co2.c — SCD41 NDIR CO2 sensor driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives the Sensirion SCD41 photoacoustic NDIR CO2 sensor over I2C.
 * Uses single-shot mode for low-power operation: trigger a measurement,
 * wait 5 s, read 9 bytes (CO2, T, RH), apply CRC-8.
 */

#include "co2.h"
#include "../board.h"
#include "../registers.h"

/* ---- I2C low-level ---- */
static void i2c_wait_tx_empty(void) {
    while (!(I2C1->ISR & I2C_ISR_TXE)) { }
}
static void i2c_wait_txis(void) {
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
}
static void i2c_wait_rxne(void) {
    while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
}
static int i2c_check_nack(void) {
    if (I2C1->ISR & I2C_ISR_NACKF) {
        I2C1->ICR = I2C_ISR_NACKF;
        return -1;
    }
    return 0;
}

static int i2c_write_cmd(uint8_t addr, uint16_t cmd) {
    I2C1->CR2 = (addr << 1) | (2 << 16) | (1 << 25) /* AUTOEND */
               | (1 << 13) /* START */;
    i2c_wait_txis();
    I2C1->TXDR = (cmd >> 8) & 0xFF;
    i2c_wait_txis();
    I2C1->TXDR = cmd & 0xFF;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return i2c_check_nack();
}

static int i2c_read_bytes(uint8_t addr, uint8_t *buf, uint16_t n) {
    I2C1->CR2 = (addr << 1) | (n << 16) | (1 << 25) | (1 << 10) /* READ */ | (1 << 13);
    for (uint16_t i = 0; i < n; i++) {
        i2c_wait_rxne();
        buf[i] = (uint8_t)I2C1->RXDR;
    }
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;
    return i2c_check_nack();
}

/* ---- CRC-8 (Sensirion polynomial 0x31, init 0xFF) ---- */
static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x31);
            else            crc <<= 1;
        }
    }
    return crc;
}

/* ---- Public API ---- */

int co2_init(void) {
    /* Power on the sensor via the board supply gate */
    co2_power_on();
    delay_ms(SCD41_WARMUP_MS);

    /* Reinit command — restores factory calibration */
    if (i2c_write_cmd(CO2_I2C_ADDR, SCD41_CMD_REINIT) < 0) return -1;
    delay_ms(20);

    /* Stop any periodic measurement mode that may be active */
    if (i2c_write_cmd(CO2_I2C_ADDR, SCD41_CMD_STOP) < 0) return -1;
    delay_ms(500);  /* stop takes 500 ms */

    return 0;
}

int co2_trigger_single_shot(void) {
    return i2c_write_cmd(CO2_I2C_ADDR, SCD41_CMD_SINGLE_SHOT);
}

int co2_read(co2_meas_t *out) {
    uint8_t buf[9];
    if (i2c_write_cmd(CO2_I2C_ADDR, SCD41_CMD_READ_MEAS) < 0) return -1;
    delay_ms(5);
    if (i2c_read_bytes(CO2_I2C_ADDR, buf, 9) < 0) return -1;

    /* Verify CRC on each triplet */
    if (crc8(&buf[0], 2) != buf[2]) return -2;
    if (crc8(&buf[3], 2) != buf[5]) return -2;
    if (crc8(&buf[6], 2) != buf[8]) return -2;

    uint16_t co2_raw  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t t_raw    = ((uint16_t)buf[3] << 8) | buf[4];
    uint16_t rh_raw   = ((uint16_t)buf[6] << 8) | buf[7];

    out->co2_ppm     = co2_raw;
    out->temperature = (int16_t)((int32_t)t_raw * 175 - 45000) / 100; /* -45..+130 C */
    out->humidity    = (int16_t)((int32_t)rh_raw * 1000 - 6000) / 100;  /* 0..100 % */

    return 0;
}

int co2_measure_blocking(co2_meas_t *out) {
    int rc = co2_trigger_single_shot();
    if (rc < 0) return rc;
    delay_ms(SCD41_MEAS_TIME_MS + 100);
    return co2_read(out);
}

void co2_power_off(void) {
    GPIOC->BSRR = (1 << (PC1__CO2_SUPPLY_EN + 16));  /* reset -> gate off */
}

void co2_power_on(void) {
    GPIOC->BSRR = (1 << PC1__CO2_SUPPLY_EN);          /* set -> gate on */
    delay_ms(5);
}