/*
 * power.c — Battery management (MAX17048 + TP4056)
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * The MAX17048 fuel gauge monitors the 1800 mAh Li-ion battery over I2C.
 * The TP4056 linear charger is controlled via PD2 (charge enable) and
 * its status is readable via the CHRG pin (simplified as GPIO read).
 */

#include "power.h"
#include "registers.h"
#include <string.h>

/* I2C1 register accessors */
#define I2C1_CR1   I2C_REG(I2C1_BASE, I2C_CR1)
#define I2C1_CR2   I2C_REG(I2C1_BASE, I2C_CR2)
#define I2C1_ISR   I2C_REG(I2C1_BASE, I2C_ISR)
#define I2C1_TXDR  I2C_REG(I2C1_BASE, I2C_TXDR)
#define I2C1_RXDR  I2C_REG(I2C1_BASE, I2C_RXDR)

/* MAX17048 I2C address (7-bit: 0x36 → 8-bit: 0x6C) */
#define MAX17048_ADDR    0x6C
#define MAX17048_REG_VCELL  0x02
#define MAX17048_REG_SOC    0x04
#define MAX17048_REG_VERSION 0x08
#define MAX17048_REG_HIBRT  0x0A
#define MAX17048_REG_CONFIG 0x0C
#define MAX17048_REG_STATUS 0x1E

#define CHARGE_EN_PIN  BIT(2)  /* PD2 */
#define CHARGE_LED     BIT(5)  /* PD5 */

static uint8_t last_battery = 100;
static bool charging = false;

/* ========================================================================
 * I2C1 basic read/write
 * ======================================================================== */
static int i2c_write_reg(uint8_t addr, uint8_t reg, uint16_t val) {
    /* Start + address + register + data + stop (simplified) */
    I2C1_CR2 = ((uint32_t)addr << 1) | (3 << 16) | (1 << 13);  /* 3 bytes, auto-end */
    while (!(I2C1_ISR & BIT(0))) { }  /* TXIS */
    I2C1_TXDR = reg;
    while (!(I2C1_ISR & BIT(1))) { }  /* TXIS */
    I2C1_TXDR = (val >> 8) & 0xFF;
    while (!(I2C1_ISR & BIT(1))) { }  /* TXIS */
    I2C1_TXDR = val & 0xFF;
    while (!(I2C1_ISR & BIT(6))) { }  /* STOPF */
    I2C1_ISR = BIT(6);  /* clear STOPF */
    return 0;
}

static int i2c_read_reg(uint8_t addr, uint8_t reg, uint16_t *val) {
    /* Write register address, then read 2 bytes */
    I2C1_CR2 = ((uint32_t)addr << 1) | (1 << 16) | (1 << 10);  /* 1 byte, no end */
    while (!(I2C1_ISR & BIT(0))) { }
    I2C1_TXDR = reg;
    while (!(I2C1_ISR & BIT(6))) { }  /* wait transfer (TC, not STOPF in restart) */

    /* Repeated start for read */
    I2C1_CR2 = ((uint32_t)addr << 1) | (2 << 16) | (1 << 13) | (1 << 10);  /* read 2, auto-end */
    while (!(I2C1_ISR & BIT(2))) { }  /* RXNE */
    uint8_t hi = I2C1_RXDR;
    while (!(I2C1_ISR & BIT(2))) { }
    uint8_t lo = I2C1_RXDR;
    while (!(I2C1_ISR & BIT(6))) { }
    I2C1_ISR = BIT(6);

    *val = ((uint16_t)hi << 8) | lo;
    return 0;
}

/* ========================================================================
 * Initialize power management
 * ======================================================================== */
void power_mgmt_init(void) {
    /* Configure I2C1: 100 kHz standard mode */
    I2C1_CR1 = 0;
    I2C1_CR1 = (0x4 << 0)   /* TIMINGR (simplified) */
             | (1 << 0);    /* PE */

    /* Configure MAX17048: quick-start, enable SOC alerts */
    i2c_write_reg(MAX17048_ADDR, MAX17048_REG_CONFIG, 0x0100);
    i2c_write_reg(MAX17048_ADDR, MAX17048_REG_STATUS, 0x0100);

    /* Enable charging by default */
    power_mgmt_enable_charge(true);

    last_battery = 100;
}

/* ========================================================================
 * Read battery state of charge (percentage)
 * ======================================================================== */
uint8_t power_mgmt_battery_pct(void) {
    uint16_t soc_raw;
    if (i2c_read_reg(MAX17048_ADDR, MAX17048_REG_SOC, &soc_raw) != 0) {
        return last_battery;  /* fallback to cached value */
    }
    /* MAX17048 SOC register: upper byte = integer %, lower = fractional */
    float soc = (float)soc_raw / 256.0f;
    if (soc > 100.0f) soc = 100.0f;
    if (soc < 0.0f) soc = 0.0f;
    last_battery = (uint8_t)soc;
    return last_battery;
}

/* ========================================================================
 * Check if charging
 * ======================================================================== */
bool power_mgmt_is_charging(void) {
    /* Read charge status pin (simplified) */
    uint32_t idr = GPIO_REG(GPIOD_BASE, GPIO_IDR_OFF);
    return !(idr & CHARGE_LED);  /* active low = charging */
}

/* ========================================================================
 * Enable / disable charging
 * ======================================================================== */
void power_mgmt_enable_charge(bool en) {
    if (en) {
        GPIO_REG(GPIOD_BASE, GPIO_BSRR_OFF) = CHARGE_EN_PIN;  /* high = enable */
    } else {
        GPIO_REG(GPIOD_BASE, GPIO_BSRR_OFF) = CHARGE_EN_PIN << 16;  /* low = disable */
    }
    charging = en;
}

/* ========================================================================
 * Poll power management (called from main loop)
 * ======================================================================== */
void power_mgmt_poll(device_ctx_t *ctx) {
    uint8_t bat = power_mgmt_battery_pct();
    ctx->battery_pct = bat;

    /* Low battery warning */
    if (bat < 15) {
        ctx->error_flags |= ERR_BATTERY_LOW;
        if (bat < 5) {
            /* Critical: shutdown */
            power_mgmt_shutdown();
        }
    } else {
        ctx->error_flags &= ~ERR_BATTERY_LOW;
    }

    /* Temperature monitoring would go here (via internal MCU temperature sensor) */
    /* Simplified: no overtemp detection in this stub */
}

/* ========================================================================
 * Graceful shutdown
 * ======================================================================== */
void power_mgmt_shutdown(void) {
    /* Disable charging */
    power_mgmt_enable_charge(false);

    /* Turn off main power */
    GPIO_REG(GPIOE_BASE, GPIO_BSRR_OFF) = BIT(8) << 16;  /* PE8 low = power off */

    /* Wait for power to drop */
    while (1) { __asm volatile ("wfi"); }
}