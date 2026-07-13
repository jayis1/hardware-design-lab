/*
 * power.c — Power management, watchdog, and RTC for HivePulse
 *
 * Manages the BQ25895 solar charge controller, battery monitoring,
 * independent watchdog (IWDG), and RTC for timekeeping and wakeup alarms.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "power.h"
#include "board.h"
#include "registers.h"
#include <string.h>

/* ---- BQ25895 I2C Address ---- */
#define BQ25895_I2C_ADDR    0x6A

/* BQ25895 register addresses */
#define BQ25895_REG_INPUT_CTRL   0x00
#define BQ25895_REG_CTRL0        0x01
#define BQ25895_REG_CTRL1        0x02
#define BQ25895_REG_CTRL2        0x03
#define BQ25895_REG_CHG_CTRL0    0x04
#define BQ25895_REG_CHG_CTRL1    0x05
#define BQ25895_REG_CHG_CTRL2    0x06
#define BQ25895_REG_VOLTAGE      0x0E  /* Battery voltage ADC */
#define BQ25895_REG_VINDPM       0x0D
#define BQ25895_REG_ADC_CTRL     0x09
#define BQ25895_REG_ADC_VBUS     0x11
#define BQ25895_REG_ADC_IBAT     0x12
#define BQ25895_REG_FAULT        0x0C
#define BQ25895_REG_STATUS       0x0B

/* RTC backup register base (20 registers × 4 bytes = 80 bytes available) */
#define RTC_BKP_BASE   (RTC_BASE + 0x50)  /* BKP0R offset */
#define RTC_BKP_COUNT  20

/* ---- Internal State ---- */
static power_mode_t current_mode = POWER_MODE_ACTIVE;
static bool rtc_initialized = false;

/* ---- I2C1 Helper (reuses sensors.c I2C1 bus) ---- */
static int i2c1_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    uint32_t timeout = 50000;
    while ((I2C1->ISR & I2C_ISR_BUSY) && timeout--);

    I2C1->CR2 = ((uint32_t)addr << 1) | (2U << 16) | (1U << 25) | (1U << 13);
    for (int i = 0; i < 2; i++) {
        while (!(I2C1->ISR & I2C_ISR_TXE) && timeout--);
        if (timeout == 0) return -1;
        I2C1->TXDR = buf[i];
    }
    timeout = 50000;
    while (!(I2C1->ISR & (1U << 6)) && timeout--);
    return (timeout == 0) ? -1 : 0;
}

static int i2c1_read_reg(uint8_t addr, uint8_t reg, uint8_t *val)
{
    uint32_t timeout = 50000;
    while ((I2C1->ISR & I2C_ISR_BUSY) && timeout--;

    /* Write register address */
    I2C1->CR2 = ((uint32_t)addr << 1) | (1U << 16) | (1U << 25) | (1U << 13);
    while (!(I2C1->ISR & I2C_ISR_TXE) && timeout--);
    I2C1->TXDR = reg;
    timeout = 50000;
    while (!(I2C1->ISR & (1U << 6)) && timeout--;

    /* Read 1 byte */
    I2C1->CR2 = ((uint32_t)addr << 1) | (1U << 0) | (1U << 16) |
                (1U << 25) | (1U << 13);
    while (!(I2C1->ISR & I2C_ISR_RXNE) && timeout--);
    *val = (uint8_t)I2C1->RXDR;
    return (timeout == 0) ? -1 : 0;
}

/* ---- BQ25895 Functions ---- */
static int bq25895_init(void)
{
    uint8_t reg_val;

    /* Read input control register to verify chip is present */
    if (i2c1_read_reg(BQ25895_I2C_ADDR, BQ25895_REG_INPUT_CTRL, &reg_val) != 0)
        return -1;

    /* Configure for LiFePO4 battery:
     * - Charge voltage: 3.65V (LiFePO4 full charge)
     * - Charge current: 1.0A (default, safe for 6000mAh cell)
     * - Input voltage limit: 4.5V (solar panel min)
     * - MPPT tracking: enabled (VINDPM = 3.5V) */

    /* Set charge voltage to 3.65V (reg 0x04: VREG[5:2] = 0x0E for 3.65V) */
    i2c1_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_CHG_CTRL0, 0x38);

    /* Set charge current to 1024mA (reg 0x03: ICHG = 0x20 = 1024mA) */
    i2c1_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_CHG_CTRL1, 0x20);

    /* Set input voltage limit to 3.5V for MPPT (reg 0x0D: VINDPM = 0x0B) */
    i2c1_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_VINDPM, 0x0B);

    /* Enable ADC for continuous battery voltage monitoring */
    i2c1_write_reg(BQ25895_I2C_ADDR, BQ25895_REG_ADC_CTRL, 0x80);

    return 0;
}

/* ---- Battery Monitoring ---- */
int power_read_battery(float *batt_mv, float *solar_mv, float *charge_ma)
{
    uint8_t batt_reg, vbus_reg, ibat_reg;

    /* Read battery voltage (reg 0x0E: bits 7:2 = voltage in 20mV steps
     * from 2304mV) */
    if (i2c1_read_reg(BQ25895_I2C_ADDR, BQ25895_REG_VOLTAGE, &batt_reg) != 0) {
        if (batt_mv) *batt_mv = 0;
        return -1;
    }
    if (batt_mv) {
        *batt_mv = 2304.0f + (float)((batt_reg >> 2) & 0x3F) * 20.0f;
    }

    /* Read VBUS (solar) voltage (reg 0x11: bits 7:2 = voltage in 100mV steps
     * from 2.6V) */
    if (i2c1_read_reg(BQ25895_I2C_ADDR, BQ25895_REG_ADC_VBUS, &vbus_reg) == 0) {
        if (solar_mv) {
            *solar_mv = 2600.0f + (float)((vbus_reg >> 2) & 0x3F) * 100.0f;
        }
    } else {
        if (solar_mv) *solar_mv = 0;
    }

    /* Read charge current (reg 0x12: bits 7:1 = current in 50mA steps) */
    if (i2c1_read_reg(BQ25895_I2C_ADDR, BQ25895_REG_ADC_IBAT, &ibat_reg) == 0) {
        if (charge_ma) {
            *charge_ma = (float)((ibat_reg >> 1) & 0x7F) * 50.0f;
        }
    } else {
        if (charge_ma) *charge_ma = 0;
    }

    return 0;
}

/* ---- Power Mode Management ---- */
void power_set_mode(power_mode_t mode)
{
    if (mode == current_mode) return;

    switch (mode) {
    case POWER_MODE_ACTIVE:
        /* All peripherals enabled, full clock speed */
        /* Ensure HCLK is 280 MHz */
        current_mode = POWER_MODE_ACTIVE;
        break;

    case POWER_MODE_LOWPOWER:
        /* Reduce CPU clock to 140 MHz, disable unused peripherals */
        current_mode = POWER_MODE_LOWPOWER;
        break;

    case POWER_MODE_STANDBY:
        /* Enter Stop mode, wake on RTC or LoRa ping slot */
        current_mode = POWER_MODE_STANDBY;
        /* Configure EXTI for LoRa DIO1 to wake from Stop */
        EXTI->IMR1 |= (1U << 2); /* Ensure DIO1 EXTI is enabled */
        break;

    case POWER_MODE_SHUTDOWN:
        /* Deep sleep — only RTC running */
        current_mode = POWER_MODE_SHUTDOWN;
        break;
    }
}

/* ---- Independent Watchdog (IWDG) ---- */
void power_watchdog_init(uint32_t timeout_ms)
{
    /* IWDG clock = LSI (~32 kHz)
     * Prescaler /256 → 125 Hz → 8ms per tick
     * For 10s timeout: 10000/8 = 1250 reload value */
    uint32_t reload = (timeout_ms / 8);
    if (reload > 0xFFF) reload = 0xFFF;

    IWDG->KR = IWDG_KEY_ENABLE;       /* Start IWDG */
    IWDG->KR = IWDG_KEY_WRITE;        /* Enable write access */
    IWDG->PR = 0x06;                  /* Prescaler /256 (LSI=32kHz → 125Hz) */
    IWDG->RLR = reload;               /* Set reload value */
    IWDG->KR = IWDG_KEY_RELOAD;       /* Reload counter */
}

void power_watchdog_refresh(void)
{
    IWDG->KR = IWDG_KEY_RELOAD;
}

/* ---- RTC Initialization ---- */
int power_rtc_init(void)
{
    /* Enable PWR clock and disable write protection */
    RCC_APB1LENR |= (1U << 28); /* PWREN */
    *(volatile uint32_t *)(PWR_BASE + 0x00) |= (1U << 8); /* DBP: disable RTC write prot */

    /* Enable LSE (32.768 kHz crystal) */
    RCC_CR |= (1U << 8);  /* LSEON */
    uint32_t timeout = 500000;
    while (!(RCC_CR & (1U << 9)) && timeout--); /* LSERDY */
    if (timeout == 0) return -1; /* LSE failed to start */

    /* Select LSE as RTC clock source */
    *(volatile uint32_t *)(RCC_BASE + 0x44) = 0x00000100; /* BDCR: RTCSEL=LSE */

    /* Enable RTC clock */
    *(volatile uint32_t *)(RCC_BASE + 0x44) |= (1U << 15); /* RTCEN */

    /* Unlock RTC write protection */
    RTC->WPR = RTC_WPR_KEY1;
    RTC->WPR = RTC_WPR_KEY2;

    /* Enter initialization mode */
    RTC->ISR |= RTC_ISR_INIT;
    timeout = 500000;
    while (!(RTC->ISR & RTC_ISR_INIT) && timeout--);

    /* Configure prescalers for 1 Hz output
     * PREDIV_A = 127, PREDIV_S = 255
     * (32768 / 128) / 256 = 1 Hz */
    RTC->PRER = (127U << 16) | 255U;

    /* Exit initialization mode */
    RTC->ISR &= ~RTC_ISR_INIT;

    /* Lock write protection */
    RTC->WPR = 0xFF;

    rtc_initialized = true;
    return 0;
}

/* ---- RTC Wakeup Alarm ---- */
void power_set_rtc_wakeup(uint32_t seconds)
{
    if (!rtc_initialized) return;

    RTC->WPR = RTC_WPR_KEY1;
    RTC->WPR = RTC_WPR_KEY2;

    /* Disable wakeup timer */
    RTC->CR &= ~(1U << 10); /* WUTE */
    uint32_t timeout = 500000;
    while (!(RTC->ISR & (1U << 2)) && timeout--); /* WUTWF */

    /* Set wakeup timer value (in seconds) */
    RTC->WUTR = seconds;

    /* Select wakeup clock = RTCCLK / 2^16 = 0.5 Hz → 2 second resolution
     * WUCKSEL[2:0] = 011 = CK_SPRE (1Hz) → 1 second resolution */
    RTC->CR &= ~(0x7U << 0);
    RTC->CR |= (0x3U << 0); /* CK_SPRE */

    /* Enable wakeup timer and interrupt */
    RTC->CR |= (1U << 10); /* WUTE */
    RTC->CR |= (1U << 14); /* WUTIE */

    RTC->WPR = 0xFF;
}

/* ---- RTC Backup Registers (state persistence) ---- */
int power_save_state_to_rtc(const void *state, uint32_t size)
{
    if (size > RTC_BKP_COUNT * 4) return -1; /* Too large */

    const uint32_t *src = (const uint32_t *)state;
    volatile uint32_t *bkp = (volatile uint32_t *)RTC_BKP_BASE;
    int words = (size + 3) / 4;

    /* Enable DBP (disable backup domain write protection) */
    *(volatile uint32_t *)(PWR_BASE + 0x00) |= (1U << 8);

    for (int i = 0; i < words; i++) {
        bkp[i] = src[i];
    }

    /* Also store size in the last backup register for validation */
    bkp[RTC_BKP_COUNT - 1] = size;

    return 0;
}

int power_restore_state_from_rtc(void *state, uint32_t size)
{
    volatile uint32_t *bkp = (volatile uint32_t *)RTC_BKP_BASE;

    /* Check if stored size matches */
    uint32_t stored_size = bkp[RTC_BKP_COUNT - 1];
    if (stored_size != size || stored_size == 0 || stored_size == 0xFFFFFFFF) {
        return -1; /* No valid stored state */
    }

    uint32_t *dst = (uint32_t *)state;
    int words = (size + 3) / 4;

    for (int i = 0; i < words; i++) {
        dst[i] = bkp[i];
    }

    return 0;
}

uint32_t power_get_rtc_timestamp(void)
{
    /* Read RTC time register (BCD format)
     * TR: bits [5:0]=SU, [6:4]=ST (seconds tens)
     *     bits [13:8]=MN, [14]=MT (minutes)
     *     bits [21:16]=HU, [22]=HT (hours)
     * For simplicity, we return a rough Unix-like timestamp.
     * In production, combine date + time registers and convert to epoch. */

    /* Wait for register sync */
    RTC->ISR &= ~RTC_ISR_RSF;
    uint32_t timeout = 100000;
    while (!(RTC->ISR & RTC_ISR_RSF) && timeout--);

    uint32_t tr = RTC->TR;
    uint32_t dr = RTC->DR;

    /* Extract BCD fields and convert to binary */
    uint8_t seconds = ((tr & 0xF) * 1) + (((tr >> 4) & 0x7) * 10);
    uint8_t minutes = (((tr >> 8) & 0xF) * 1) + (((tr >> 12) & 0x7) * 10);
    uint8_t hours   = (((tr >> 16) & 0xF) * 1) + (((tr >> 20) & 0x3) * 10);
    uint8_t day     = ((dr & 0xF) * 1) + (((dr >> 4) & 0x3) * 10);
    uint8_t month   = (((dr >> 8) & 0xF) * 1) + (((dr >> 12) & 0x1) * 10);
    uint8_t year    = (((dr >> 16) & 0xF) * 1) + (((dr >> 20) & 0xF) * 10);

    /* Rough epoch calculation (not accounting for leap years, etc.) */
    uint32_t epoch = (year * 365 + month * 30 + day) * 86400 +
                     hours * 3600 + minutes * 60 + seconds;
    return epoch;
}

/* ---- Top-Level Power Management Init ---- */
int power_management_init(void)
{
    int ret = 0;

    /* Initialize BQ25895 solar charge controller */
    ret |= bq25895_init();

    /* Initialize ADC for solar/battery voltage sensing (backup path) */
    RCC_AHB4ENR |= RCC_AHB4ENR_ADC12EN;

    /* Configure PA0, PA1, PA2 as analog inputs for ADC */
    GPIOA->MODER &= ~(0x3U << 0);  /* PA0 */
    GPIOA->MODER |= (GPIO_MODE_ANALOG << 0);
    GPIOA->MODER &= ~(0x3U << 2);  /* PA1 */
    GPIOA->MODER |= (GPIO_MODE_ANALOG << 2);
    GPIOA->MODER &= ~(0x3U << 4);  /* PA2 */
    GPIOA->MODER |= (GPIO_MODE_ANALOG << 4);

    /* Initialize RTC if not already done */
    if (!rtc_initialized) {
        ret |= power_rtc_init();
    }

    return ret;
}