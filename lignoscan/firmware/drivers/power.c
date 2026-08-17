/*
 * power.c — Power Management Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Manages the MAX17048 fuel gauge (I²C), MCP73871 charger status,
 * and low-power sleep modes to maximize battery life.
 */

#include "power.h"
#include "board.h"

static uint32_t last_activity_ms = 0;
static uint8_t battery_percent = 100;

/* ---- I²C read from MAX17048 ---- */
static uint8_t fuel_read_reg(uint8_t reg, uint16_t *val) {
    /* Send register address */
    FUEL_I2C->CR2 = (FUEL_ADDR << 1) | (1U << 16) | (1U << 25); /* 1 byte write, AUTOEND */
    FUEL_I2C->TXDR = reg;
    while (!(FUEL_I2C->ISR & I2C_ISR_TC)) {
        if (FUEL_I2C->ISR & I2C_ISR_NACKF) return 0;
    }

    /* Read 2 bytes */
    FUEL_I2C->CR2 = (FUEL_ADDR << 1) | (2U << 16) | (1U << 10) | (1U << 25); /* 2 byte read, RD_WRN, AUTOEND */
    while (!(FUEL_I2C->ISR & I2C_ISR_RXNE)) { }
    uint8_t hi = (uint8_t)FUEL_I2C->RXDR;
    while (!(FUEL_I2C->ISR & I2C_ISR_RXNE)) { }
    uint8_t lo = (uint8_t)FUEL_I2C->RXDR;

    *val = ((uint16_t)hi << 8) | lo;
    return 1;
}

/* ---- Initialize power management ---- */
void power_init(void) {
    /* Enable I²C1 clock */
    RCC_APB1LENR |= RCC_APB1LENR_I2C1EN;

    /* Configure I²C1 timing for 100 kHz at 140 MHz APB1 clock */
    FUEL_I2C->TIMINGR = 0x10909CECUL;  /* 100 kHz standard mode */
    FUEL_I2C->CR1 = I2C_CR1_PE;  /* Enable peripheral */

    last_activity_ms = millis();
    battery_percent = 100;
}

/* ---- Read battery percentage from MAX17048 ---- */
uint8_t power_get_battery_percent(void) {
    return battery_percent;
}

/* ---- Update battery gauge reading ---- */
void power_update_gauge(void) {
    uint16_t soc;
    if (fuel_read_reg(0x04, &soc)) {  /* SOC register */
        /* MAX17048 returns SoC as a fixed-point number:
         * upper byte = integer %, lower byte = fractional %
         * We use just the integer part */
        battery_percent = (uint8_t)(soc >> 8);
    }
}

/* ---- Check if currently charging ---- */
int power_is_charging(void) {
    /* MCP73871 STAT1 pin: low = charging, high = not charging */
    return !GPIO_GET(CHG_STAT1, CHG_STAT1_PIN);
}

/* ---- Enter low-power sleep mode ---- */
void power_enter_sleep(void) {
    /* Disable HV, AFE, and TDC to save power */
    hv_disable();
    afe_power_down();
    tdc_power_down();

    /* Enter WFI (Wait For Interrupt) mode.
     * SysTick and GPIO interrupts will wake the MCU. */
    __asm volatile ("wfi");

    /* On wake, re-enable peripherals */
    tdc_init();
    afe_init();
    hv_init();

    last_activity_ms = millis();
}

/* ---- Get seconds since last activity ---- */
uint32_t power_idle_time_seconds(void) {
    return (millis() - last_activity_ms) / 1000;
}

/* EOF — power.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */