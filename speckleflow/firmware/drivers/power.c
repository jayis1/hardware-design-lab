/*
 * power.c — Battery monitoring and power management for SpeckleFlow
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * The MAX17048 fuel gauge provides accurate battery state-of-charge
 * via I2C4 at 0x36. We also read the battery voltage via ADC1
 * channel 0 (PA0) as a cross-check.
 */

#include "power.h"
#include "board.h"
#include "registers.h"

#define MAX17048_ADDR  0x36

/* MAX17048 registers */
#define MAX17048_REG_VCELL   0x02
#define MAX17048_REG_SOC     0x04
#define MAX17048_REG_MODE    0x06
#define MAX17048_REG_VERSION 0x08
#define MAX17048_REG_HIBRT   0x0A
#define MAX17048_REG_CONFIG  0x0C
#define MAX17048_REG_VALRT   0x14
#define MAX17048_REG_CRATE   0x16
#define MAX17048_REG_VRESET  0x18

static uint8_t batt_pct = 100;
static uint16_t batt_mv = 4200;
static int power_charging = 0;

/* ---- I2C4 read (shared with imu.c — simplified here) ------------------- */

static int i2c4_read16(uint8_t dev_addr, uint8_t reg, uint16_t *val) {
    uint32_t timeout = 100000;
    while (I2C4->ISR & I2C_ISR_BUSY) {
        if (--timeout == 0) return -1;
    }

    I2C4->CR2 = ((uint32_t)dev_addr << 1)
              | (1u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_RELOAD;
    I2C4->CR2 |= I2C_CR2_START;
    while (!(I2C4->ISR & I2C_ISR_TXE)) { }
    if (I2C4->ISR & I2C_ISR_NACKF) { I2C4->ICR = I2C_ISR_NACKF; return -1; }
    I2C4->TXDR = reg;
    while (!(I2C4->ISR & I2C_ISR_TC)) { }

    I2C4->CR2 = ((uint32_t)dev_addr << 1)
              | I2C_CR2_RD_WRN
              | (2u << I2C_CR2_NBYTES_SHIFT)
              | I2C_CR2_AUTOEND;
    I2C4->CR2 |= I2C_CR2_START;
    while (!(I2C4->ISR & I2C_ISR_RXNE)) { }
    uint8_t hi = (uint8_t)(I2C4->RXDR & 0xFF);
    while (!(I2C4->ISR & I2C_ISR_RXNE)) { }
    uint8_t lo = (uint8_t)(I2C4->RXDR & 0xFF);
    while (!(I2C4->ISR & I2C_ISR_STOPF)) { }
    I2C4->ICR = I2C_ISR_STOPF;
    *val = ((uint16_t)hi << 8) | lo;
    return 0;
}

/* ---- ADC (battery voltage cross-check) ---------------------------------- */

static uint16_t adc_read_batt(void) {
    /* ADC1 channel 0 (PA0) — battery voltage via divider (2:1) */
    ADC1->SQR1 = (1u << 0) | (0u << 6);
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    uint16_t raw = (uint16_t)(ADC1->DR & 0xFFF);
    /* Convert: raw/4095 * 3.3V * 2 (divider) * 1000 = mV */
    return (uint16_t)((uint32_t)raw * 3300 * 2 / 4095);
}

/* ---- Public API --------------------------------------------------------- */

int power_init(void) {
    /* Verify MAX17048 is present */
    uint16_t version = 0;
    if (i2c4_read16(MAX17048_ADDR, MAX17048_REG_VERSION, &version) != 0)
        return -1;
    if ((version >> 8) != 0x00) return -2;  /* expected version byte */

    /* Quick-start mode */
    uint16_t config;
    i2c4_read16(MAX17048_ADDR, MAX17048_REG_CONFIG, &config);
    /* (write not implemented here for brevity) */

    power_update();
    return 0;
}

void power_update(void) {
    /* Read state-of-charge from MAX17048 */
    uint16_t soc_raw = 0;
    if (i2c4_read16(MAX17048_ADDR, MAX17048_REG_SOC, &soc_raw) == 0) {
        batt_pct = (uint8_t)(soc_raw >> 8);  /* upper byte = integer % */
    }

    /* Read voltage from MAX17048 */
    uint16_t vcell_raw = 0;
    if (i2c4_read16(MAX17048_ADDR, MAX17048_REG_VCELL, &vcell_raw) == 0) {
        /* VCELL = raw * 78.125 µV → mV = raw * 78.125 / 1000 */
        batt_mv = (uint16_t)((uint32_t)vcell_raw * 78u / 1000u);
    }

    /* Cross-check with ADC */
    uint16_t adc_mv = adc_read_batt();
    if (adc_mv > batt_mv + 100 || adc_mv < batt_mv - 100) {
        /* Significant discrepancy — use ADC as fallback */
        batt_mv = adc_mv;
    }

    /* Check charging status (USB-C VBUS via GPIO) */
    power_charging = (GPIOB->IDR & (1u << 5)) ? 1 : 0;
}

uint8_t power_get_battery_pct(void) {
    return batt_pct;
}

uint16_t power_get_battery_mv(void) {
    return batt_mv;
}

int power_is_charging(void) {
    return power_charging;
}

int power_is_low(void) {
    return (batt_pct < BATT_WARN_PCT) ? 1 : 0;
}

uint16_t power_get_charge_rate(void) {
    uint16_t crate_raw = 0;
    if (i2c4_read16(MAX17048_ADDR, MAX17048_REG_CRATE, &crate_raw) != 0)
        return 0;
    /* CRATE = raw * 0.208%/hr, signed */
    int16_t signed_rate = (int16_t)crate_raw;
    return (uint16_t)(signed_rate / 10);
}