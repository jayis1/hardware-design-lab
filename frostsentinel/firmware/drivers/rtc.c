/*
 * drivers/rtc.c — RV-3028-C7 real-time clock driver
 *
 * The RV-3028-C7 is an ultra-low-power (45 nA) I²C RTC with an
 * internal TCXO (±1 ppm) and a backup battery (CR1200).  It provides
 * a 1 Hz or 1 kHz clock output and two alarm timers.  FrostSentinel
 * uses the 1 kHz output as the system tick (via TIM6) and the alarm
 * to wake from Stop2 sleep at each sample interval.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "rtc.h"
#include "../board.h"

/* RV-3028-C7 register map */
#define RV3028_REG_SECONDS    0x00
#define RV3028_REG_MINUTES    0x01
#define RV3028_REG_HOURS      0x02
#define RV3028_REG_DATE       0x03
#define RV3028_REG_WEEKDAY    0x04
#define RV3028_REG_MONTH      0x05
#define RV3028_REG_YEARS      0x06
#define RV3028_REG_STATUS     0x07
#define RV3028_REG_CTRL1      0x08
#define RV3028_REG_CTRL2      0x09
#define RV3028_REG_ALARM_SEC  0x0A
#define RV3028_REG_TIMER_A    0x10
#define RV3028_REG_TIMER_B    0x11
#define RV3028_REG_INT_MASK   0x70

/* Status bits */
#define RV3028_STATUS_POR     0x20   /* power-on reset */
#define RV3028_STATUS_AF      0x40   /* alarm flag */
#define RV3028_STATUS_TF      0x80   /* timer flag */

/* Control bits */
#define RV3028_CTRL1_EERD     0x04   /* EEPROM refresh done */
#define RV3028_CTRL1_TE       0x80   /* timer enable */

/* ------------------------------------------------------------------ */
/*  I²C primitives (shared bus, single-byte reg addressing)           */
/* ------------------------------------------------------------------ */
static int rtc_write(uint8_t reg, uint8_t val)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)I2C_ADDR_RV3028 << 1) | (2u << 16) |
               I2C_CR2_START | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = val;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    return (to == 0) ? -1 : 0;
}

static int rtc_read(uint8_t reg, uint8_t *out)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)I2C_ADDR_RV3028 << 1) | (1u << 16) | I2C_CR2_START;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TC) && to--) ;

    i2c->CR2 = ((uint32_t)I2C_ADDR_RV3028 << 1) | (1u << 16) |
               I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
    *out = i2c->RXDR & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    return (to == 0) ? -1 : 0;
}

static int rtc_read_multi(uint8_t reg, uint8_t *out, uint8_t len)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)I2C_ADDR_RV3028 << 1) | (1u << 16) | I2C_CR2_START;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TC) && to--) ;

    i2c->CR2 = ((uint32_t)I2C_ADDR_RV3028 << 1) | ((uint32_t)len << 16) |
               I2C_CR2_START | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    for (uint8_t i = 0; i < len; i++) {
        to = 0x10000;
        while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
        out[i] = i2c->RXDR & 0xFF;
    }
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    return (to == 0) ? -1 : 0;
}

/* ------------------------------------------------------------------ */
/*  BCD ↔ binary conversion                                            */
/* ------------------------------------------------------------------ */
static uint8_t bcd_to_bin(uint8_t bcd)
{
    return ((bcd >> 4) & 0x0F) * 10 + (bcd & 0x0F);
}

static uint8_t bin_to_bcd(uint8_t bin)
{
    return ((bin / 10) << 4) | (bin % 10);
}

/* ------------------------------------------------------------------ */
/*  Public: initialize RTC                                             */
/* ------------------------------------------------------------------ */
int rtc_init(void)
{
    /* Check for power-on reset */
    uint8_t status = 0;
    if (rtc_read(RV3028_REG_STATUS, &status) < 0) return -1;

    if (status & RV3028_STATUS_POR) {
        /* First power-up: set default time to 2026-01-01 00:00:00 */
        rtc_write(RV3028_REG_SECONDS, bin_to_bcd(0));
        rtc_write(RV3028_REG_MINUTES, bin_to_bcd(0));
        rtc_write(RV3028_REG_HOURS,   bin_to_bcd(0));
        rtc_write(RV3028_REG_DATE,    bin_to_bcd(1));
        rtc_write(RV3028_REG_WEEKDAY, bin_to_bcd(4));  /* Thursday */
        rtc_write(RV3028_REG_MONTH,   bin_to_bcd(1));
        rtc_write(RV3028_REG_YEARS,   bin_to_bcd(26));
        /* Clear POR flag */
        rtc_write(RV3028_REG_STATUS, status & ~RV3028_STATUS_POR);
    }

    /* Enable 1 kHz clock output on CLKOUT pin (for TIM6 tick) */
    rtc_write(RV3028_REG_INT_MASK, 0x03);  /* CLKOUT = 1 kHz */

    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public: get current time as Unix epoch seconds                     */
/* ------------------------------------------------------------------ */
uint32_t rtc_get_seconds(void)
{
    uint8_t regs[7];
    if (rtc_read_multi(RV3028_REG_SECONDS, regs, 7) < 0) return 0;

    uint8_t sec  = bcd_to_bin(regs[0]);
    uint8_t min  = bcd_to_bin(regs[1]);
    uint8_t hour = bcd_to_bin(regs[2]);
    uint8_t day  = bcd_to_bin(regs[3]);
    uint8_t mon  = bcd_to_bin(regs[5]);
    uint8_t year = bcd_to_bin(regs[6]);  /* 00-99 → 2000-2099 */

    /* Simplified epoch (days since 1970-01-01, ignoring leap-year nuances
     * beyond the standard rule — adequate for a sensor timestamp). */
    uint32_t days = (year + 30) * 365;   /* 2000 → 30 years from 1970 */
    /* Add leap days for years 2000..year-1+2000 */
    for (uint32_t y = 2000; y < 2000 + year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) days++;
    }
    /* Days in months before current month (non-leap year approx) */
    static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (uint8_t m = 0; m < mon - 1; m++) days += dim[m];
    /* Leap day if current year is leap and past February */
    uint32_t cy = 2000 + year;
    if ((cy % 4 == 0 && cy % 100 != 0) || (cy % 400 == 0)) {
        if (mon > 2) days++;
    }
    days += day - 1;

    return days * 86400u + hour * 3600u + min * 60u + sec;
}

/* ------------------------------------------------------------------ */
/*  Public: set time from Unix epoch seconds                           */
/* ------------------------------------------------------------------ */
void rtc_set_seconds(uint32_t epoch)
{
    uint32_t secs = epoch % 60;
    uint32_t mins = (epoch / 60) % 60;
    uint32_t hrs  = (epoch / 3600) % 24;
    uint32_t days = epoch / 86400u;

    /* Compute date from days since 1970-01-01 */
    uint32_t year = 1970;
    uint32_t day_of_year = 0;
    static const uint8_t dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};

    while (1) {
        uint32_t days_in_year = 365;
        int is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (is_leap) days_in_year = 366;
        if (days < days_in_year) {
            day_of_year = days;
            break;
        }
        days -= days_in_year;
        year++;
    }

    uint32_t month = 0;
    for (uint32_t m = 0; m < 12; m++) {
        uint32_t dm = dim[m];
        if (m == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)))
            dm = 29;
        if (days < dm) { month = m + 1; break; }
        days -= dm;
    }
    uint32_t date = days + 1;

    rtc_write(RV3028_REG_SECONDS, bin_to_bcd((uint8_t)secs));
    rtc_write(RV3028_REG_MINUTES, bin_to_bcd((uint8_t)mins));
    rtc_write(RV3028_REG_HOURS,   bin_to_bcd((uint8_t)hrs));
    rtc_write(RV3028_REG_DATE,    bin_to_bcd((uint8_t)date));
    rtc_write(RV3028_REG_MONTH,   bin_to_bcd((uint8_t)month));
    rtc_write(RV3028_REG_YEARS,   bin_to_bcd((uint8_t)(year - 2000)));
}

/* ------------------------------------------------------------------ */
/*  Public: set alarm N seconds from now (for Stop2 wake)              */
/* ------------------------------------------------------------------ */
void rtc_set_alarm(uint32_t seconds_from_now)
{
    /* Use Timer A as a countdown alarm (1 Hz resolution) */
    uint8_t ctrl1 = 0;
    rtc_read(RV3028_REG_CTRL1, &ctrl1);

    /* Disable timer while configuring */
    rtc_write(RV3028_REG_CTRL1, ctrl1 & ~RV3028_CTRL1_TE);

    /* Clear timer flag */
    uint8_t status = 0;
    rtc_read(RV3028_REG_STATUS, &status);
    rtc_write(RV3028_REG_STATUS, status & ~RV3028_STATUS_TF);

    /* Set Timer A clock to 1 Hz (TD = 0b011 in CTRL2) */
    rtc_write(RV3028_REG_CTRL2, 0x03 << 4);

    /* Set Timer A value (12-bit) */
    rtc_write(RV3028_REG_TIMER_A, (uint8_t)(seconds_from_now & 0xFF));

    /* Enable timer and timer interrupt */
    rtc_write(RV3028_REG_CTRL1, ctrl1 | RV3028_CTRL1_TE);
}

/* ------------------------------------------------------------------ */
/*  Public: check and clear alarm flag                                 */
/* ------------------------------------------------------------------ */
int rtc_alarm_triggered(void)
{
    uint8_t status = 0;
    rtc_read(RV3028_REG_STATUS, &status);
    if (status & RV3028_STATUS_TF) {
        rtc_write(RV3028_REG_STATUS, status & ~RV3028_STATUS_TF);
        return 1;
    }
    return 0;
}