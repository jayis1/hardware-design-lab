/*
 * bq25895.c - TI BQ25895 MPPT charger driver (I2C3)
 * Reads battery voltage, input (solar) voltage, charge current,
 * and charge percentage. Controls charging via register config.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "bq25895.h"
#include "board.h"
#include "registers.h"

/* Register map */
#define REG_ADC_CTRL        0x02   /* I2C watchdog + ADC */
#define REG_CHARGE_STATUS   0x0B   /* charge status bits  */
#define REG_SYS_CTRL        0x03
#define REG_FAULT           0x0C
#define REG_VBUS_STAT       0x0B
#define REG_ADC_VBAT        0x0E   /* ADC battery voltage */
#define REG_ADC_VSYS        0x0F
#define REG_ADC_VBUS        0x10   /* ADC VBUS (solar)    */
#define REG_ADC_ICHGR       0x11   /* ADC charge current  */

static int i2c_write(uint8_t reg, uint8_t val)
{
    I2C_CR2(I2C3_BASE) = BQ25895_I2C_ADDR | (2U << 16) | BIT(13);
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C3_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C3_BASE) = reg;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXIS)) { }
    I2C_TXDR(I2C3_BASE) = val;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C3_BASE) = I2C_ISR_STOPF;
    return 0;
}

static int i2c_read(uint8_t reg, uint8_t *val)
{
    I2C_CR2(I2C3_BASE) = BQ25895_I2C_ADDR | (1U << 16) | BIT(10);
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C3_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C3_BASE) = reg;
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_TC)) { }
    I2C_CR2(I2C3_BASE) = BQ25895_I2C_ADDR | (1U << 16) | BIT(13) | BIT(10);
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_RXNE)) { }
    *val = (uint8_t)I2C_RXDR(I2C3_BASE);
    while (!(I2C_ISR(I2C3_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C3_BASE) = I2C_ISR_STOPF;
    return 0;
}

int bq25895_init(void)
{
    I2C_TIMINGR(I2C3_BASE) = 0x10909CEC;
    I2C_CR1(I2C3_BASE) = 1U;

    /* Enable ADC continuous, disable I2C watchdog for simplicity */
    i2c_write(REG_ADC_CTRL, 0x80 | 0x40);  /* ADC_START, ADC_RATE=1 */
    /* Set charge voltage limit to 6.6 V (2S LiFePO4 full = 6.6 V) */
    i2c_write(0x04, 0b10110110);           /* VREG = 6.6 V */
    /* Set input current limit to 2 A, MPPT on (MPP=1) */
    i2c_write(0x00, 0b01111100);           /* IINLIM = 2 A, EN_HIZ=0 */
    i2c_write(0x0D, 0x01);                 /* MPPT enable */
    return 0;
}

int bq25895_read(bq25895_status_t *out)
{
    uint8_t vbat, vbus, stat, ichgr;
    if (i2c_read(REG_ADC_VBAT, &vbat))  return -1;
    if (i2c_read(REG_ADC_VBUS, &vbus))  return -1;
    if (i2c_read(REG_CHARGE_STATUS, &stat)) return -1;
    if (i2c_read(REG_ADC_ICHGR, &ichgr)) return -1;

    /* ADC conversion: VBAT = (REG14[7:0] * 20) mV + 2304 mV */
    out->bat_mv  = (uint16_t)((vbat & 0x7F) * 20 + 2304);
    /* VBUS = (REG10[7:0] * 100) mV + 2600 mV */
    out->vin_mv  = (uint16_t)((vbus & 0x7F) * 100 + 2600);
    /* Charge current = (REG11[6:0] * 50) mA */
    out->ichg_ma = (uint16_t)((ichgr & 0x7F) * 50);

    /* Charge status: bits [5:4] = 00 not charging, 01 precharge,
     *                10 fast charge, 11 charge done */
    uint8_t chg = (stat >> 4) & 0x03;
    out->charging = (chg == 1 || chg == 2);
    out->charge_done = (chg == 3);

    /* Estimate charge % from voltage (2S LiFePO4 discharge curve) */
    if (out->bat_mv >= 6600) out->charge_pct = 100;
    else if (out->bat_mv <= 5400) out->charge_pct = 0;
    else {
        /* Linear approx between 5.4 V (0%) and 6.6 V (100%) */
        out->charge_pct = (uint8_t)(((out->bat_mv - 5400) * 100) / 1200);
    }
    return 0;
}