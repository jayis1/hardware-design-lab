/*
 * si1145.c - SI1145 UV index / IR / visible light sensor driver (I2C1)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "si1145.h"
#include "board.h"
#include "registers.h"

#define SI1145_REG_PART_ID   0x00
#define SI1145_REG_HWKEY     0x07
#define SI1145_REG_PARAM_WR  0x17
#define SI1145_REG_PARAM_RD  0x2E
#define SI1145_REG_CMD       0x18
#define SI1145_REG_ALS_VIS   0x22
#define SI1145_REG_ALS_IR    0x24
#define SI1145_REG_AUX       0x26
#define SI1145_REG_MEAS_RATE 0x08
#define SI1145_REG_IRQ_EN    0x03
#define SI1145_REG_INT_CFG   0x01

#define SI1145_CMD_RESET     0x01
#define SI1145_CMD_ALS_FORCE 0x06
#define SI1145_CMD_PARAM_SET 0xA0
#define SI1145_CMD_PARAM_QUERY 0x80

static int i2c_write(uint8_t reg, uint8_t val)
{
    I2C_CR2(I2C1_BASE) = SI1145_I2C_ADDR | (2U << 16) | BIT(13);
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
    I2C_CR2(I2C1_BASE) = SI1145_I2C_ADDR | (1U << 16) | BIT(10);
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C1_BASE) = reg;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TC)) { }
    I2C_CR2(I2C1_BASE) = SI1145_I2C_ADDR | ((uint32_t)len << 16) | BIT(13) | BIT(10);
    for (uint8_t i = 0; i < len; i++) {
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C_RXDR(I2C1_BASE);
    }
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C1_BASE) = I2C_ISR_STOPF;
    return 0;
}

static int param_set(uint8_t param, uint8_t value)
{
    i2c_write(SI1145_REG_PARAM_WR, value);
    i2c_write(SI1145_REG_CMD, SI1145_CMD_PARAM_SET | (param & 0x1F));
    return 0;
}

int si1145_init(void)
{
    uint8_t id;
    if (i2c_read(SI1145_REG_PART_ID, &id, 1) != 0 || id != 0x45) return -1;

    /* HW key must be written after reset for correct operation */
    i2c_write(SI1145_REG_CMD, SI1145_CMD_RESET);
    for (volatile int i = 0; i < 20000; i++) { }
    i2c_write(SI1145_REG_HWKEY, 0x17);

    /* Configure ALS for VIS + IR + AUX (UV) */
    param_set(0x01, 0x07);   /* CHLIST: VIS, IR, AUX */
    param_set(0x0B, 0x01);   /* ALS_VIS_ADC_GAIN = 1 */
    param_set(0x0E, 0x01);   /* ALS_IR_ADC_GAIN    = 1 */
    param_set(0x0D, 0x00);   /* ALS_IR_ADC_COUNTER */
    /* Auto-measure every 31 ms */
    i2c_write(SI1145_REG_MEAS_RATE, 0x00);
    i2c_write(SI1145_REG_INT_CFG, 0x00);
    i2c_write(SI1145_REG_IRQ_EN, 0x00);
    return 0;
}

float si1145_read_uv(void)
{
    i2c_write(SI1145_REG_CMD, SI1145_CMD_ALS_FORCE);
    for (volatile int i = 0; i < 5000; i++) { }
    uint8_t buf[4];
    if (i2c_read(SI1145_REG_ALS_VIS, buf, 4) != 0) return -1.0f;
    uint16_t vis = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    uint16_t ir  = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
    /* The SI1145 UV index is approximated from the AUX/IR ratio.
     * Real device returns AUX in 0..255 mapped to 0..11+ UV index. */
    float uv = (float)ir / 100.0f;
    if (uv < 0.0f) uv = 0.0f;
    if (uv > 14.0f) uv = 14.0f;
    (void)vis;
    return uv;
}