/**
 * @file    rv8803.c
 * @brief   RV-8803 RTC driver with battery-backed timestamp and alarms.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "rv8803.h"
#include "../board.h"
#include "../registers.h"

#define RV8803_REG_SEC         0x00
#define RV8803_REG_MIN         0x01
#define RV8803_REG_HOUR        0x02
#define RV8803_REG_WEEK        0x03
#define RV8803_REG_DAY         0x04
#define RV8803_REG_MONTH       0x05
#define RV8803_REG_YEAR        0x06
#define RV8803_REG_ALM_MIN     0x07
#define RV8803_REG_ALM_HOUR    0x08
#define RV8803_REG_ALM_WEEK    0x09
#define RV8803_REG_TIMER_CNT   0x0A
#define RV8803_REG_STATUS      0x0B
#define RV8803_REG_CTRL1       0x0C
#define RV8803_REG_CTRL2       0x0D
#define RV8803_REG_CLKOUT      0x0F

#define RV8803_I2C_ADDR_8BIT   (RV8803_I2C_ADDR << 1)

static uint8_t bcd2bin(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t bin2bcd(uint8_t bin) { return ((bin / 10) << 4) | (bin % 10); }

static int rv_i2c_write_reg(uint8_t reg, uint8_t val) {
    uint32_t t = 50000;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (RV8803_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (2 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    t = 50000; while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = reg;
    t = 50000; while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = val;
    t = 50000; while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--t == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

static int rv_i2c_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint32_t t = 50000;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (RV8803_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (1 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;
    t = 50000; while (!(I2C1->ISR & I2C_ISR_TXE)) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->TXDR = reg;
    t = 50000; while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_NACKF) { I2C1->ICR = I2C_ISR_NACKF; return -ERR_I2C_NACK; }
        if (--t == 0) return -ERR_I2C_TIMEOUT;
    }
    I2C1->ICR = I2C_ISR_STOPF;
    t = 50000;
    while (I2C1->ISR & I2C_ISR_BUSY) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = 0xFFFFFFFF;
    I2C1->CR2 = (RV8803_I2C_ADDR_8BIT << I2C_CR2_SADD_Pos)
              | (len << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START;
    for (uint8_t i = 0; i < len; i++) {
        t = 50000; while (!(I2C1->ISR & I2C_ISR_RXNE)) {
            if (I2C1->ISR & I2C_ISR_STOPF) break;
            if (--t == 0) return -ERR_I2C_TIMEOUT;
        }
        buf[i] = (uint8_t)(I2C1->RXDR & 0xFF);
    }
    t = 50000; while (!(I2C1->ISR & I2C_ISR_STOPF)) { if (--t == 0) return -ERR_I2C_TIMEOUT; }
    I2C1->ICR = I2C_ISR_STOPF;
    return 0;
}

/* Days in month for Unix timestamp conversion */
static const uint8_t days_in_mon[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

static int is_leap(uint16_t y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static uint32_t date_to_unix(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s) {
    uint32_t days = 0;
    for (uint16_t yr = 1970; yr < y; yr++) days += is_leap(yr) ? 366 : 365;
    for (uint8_t mo = 0; mo < m - 1; mo++) days += days_in_mon[mo] + ((mo == 1 && is_leap(y)) ? 1 : 0);
    days += (d - 1);
    return days * 86400UL + h * 3600UL + min * 60UL + s;
}

int rv8803_init(void) {
    uint8_t status;
    int ret = rv_i2c_read_regs(RV8803_REG_STATUS, &status, 1);
    if (ret < 0) return ret;

    /* Clear power-on reset flag */
    if (status & 0x20) {
        ret = rv_i2c_write_reg(RV8803_REG_STATUS, status & ~0x20);
        if (ret < 0) return ret;
    }

    /* Enable 24-hour mode (bit 1 in CTRL1 = 0) */
    ret = rv_i2c_write_reg(RV8803_REG_CTRL1, 0x00);
    if (ret < 0) return ret;

    /* Configure: interrupt on alarm match (INT enable) */
    ret = rv_i2c_write_reg(RV8803_REG_CTRL2, 0x00);
    if (ret < 0) return ret;

    return 0;
}

int rv8803_set_time(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t min, uint8_t s) {
    uint8_t data[7] = {
        bin2bcd(s), bin2bcd(min), bin2bcd(h), 1, /* week=Monday */
        bin2bcd(d), bin2bcd(m), bin2bcd(y - 2000)
    };
    for (int i = 0; i < 7; i++) {
        int ret = rv_i2c_write_reg(RV8803_REG_SEC + i, data[i]);
        if (ret < 0) return ret;
    }
    return 0;
}

uint32_t rv8803_read_unix(void) {
    uint8_t buf[7];
    int ret = rv_i2c_read_regs(RV8803_REG_SEC, buf, 7);
    if (ret < 0) return 0;

    uint8_t s = bcd2bin(buf[0] & 0x7F);
    uint8_t m = bcd2bin(buf[1] & 0x7F);
    uint8_t h = bcd2bin(buf[2] & 0x3F);
    uint8_t d = bcd2bin(buf[4] & 0x3F);
    uint8_t mo = bcd2bin(buf[5] & 0x1F);
    uint16_t y = bcd2bin(buf[6]) + 2000;

    return date_to_unix(y, mo, d, h, m, s);
}

int rv8803_set_alarm_s(uint32_t seconds_from_now) {
    uint32_t now = rv8803_read_unix();
    uint32_t target = now + seconds_from_now;

    /* Convert target Unix time back to BCD registers */
    uint32_t t = target;
    uint16_t y = 1970;
    while (1) {
        uint32_t ds = is_leap(y) ? 366UL * 86400UL : 365UL * 86400UL;
        if (t < ds) break;
        t -= ds; y++;
    }
    uint8_t m = 0;
    while (1) {
        uint8_t dim = days_in_mon[m] + ((m == 1 && is_leap(y)) ? 1 : 0);
        if (t < dim * 86400UL) break;
        t -= dim * 86400UL; m++;
    }
    m++;
    uint8_t d = (uint8_t)(t / 86400UL) + 1; t %= 86400UL;
    uint8_t h = (uint8_t)(t / 3600UL);      t %= 3600UL;
    uint8_t min = (uint8_t)(t / 60UL);       t %= 60UL;
    uint8_t s = (uint8_t)t;

    /* Write alarm registers (min, hour, weekday, day) */
    /* We set alarm on exact day/hour/min (ignore seconds and weekday) */
    int ret;
    ret = rv_i2c_write_reg(RV8803_REG_ALM_MIN,  bin2bcd(min));
    if (ret < 0) return ret;
    ret = rv_i2c_write_reg(RV8803_REG_ALM_HOUR, bin2bcd(h));
    if (ret < 0) return ret;
    ret = rv_i2c_write_reg(RV8803_REG_ALM_WEEK, bin2bcd(d));  /* Use week/day reg */
    if (ret < 0) return ret;

    /* Enable alarm interrupt */
    ret = rv_i2c_write_reg(RV8803_REG_CTRL2, 0x08);  /* AIE = alarm interrupt enable */
    return ret;
}