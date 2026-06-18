/*
 * sdp810.c - Sensirion SDP810 differential pressure sensor driver (I2C1)
 * Used for closed-loop blower flow control. Returns flow in L/min.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "sdp810.h"
#include "board.h"
#include "registers.h"

#define SDP810_CMD_CONTINUOUS  0x3615   /* mass flow, no averaging */

static int i2c_write16(uint16_t cmd)
{
    I2C_CR2(I2C1_BASE) = SDP810_I2C_ADDR | (2U << 16) | BIT(13);
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C1_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C1_BASE) = (cmd >> 8) & 0xFF;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_TXIS)) { }
    I2C_TXDR(I2C1_BASE) = cmd & 0xFF;
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C1_BASE) = I2C_ISR_STOPF;
    return 0;
}

static int i2c_read9(uint8_t *buf)
{
    I2C_CR2(I2C1_BASE) = SDP810_I2C_ADDR | (9U << 16) | BIT(13) | BIT(10);
    for (int i = 0; i < 9; i++) {
        while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C_RXDR(I2C1_BASE);
    }
    while (!(I2C_ISR(I2C1_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C1_BASE) = I2C_ISR_STOPF;
    return 0;
}

static uint8_t crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc;
}

int sdp810_init(void)
{
    return i2c_write16(SDP810_CMD_CONTINUOUS);
}

float sdp810_read_flow_lpm(void)
{
    uint8_t buf[9];
    if (i2c_read9(buf) != 0) return -1.0f;
    /* 9 bytes: 3x (2 data + 1 CRC). First word = raw mass flow. */
    if (crc8(buf, 2)   != buf[2])  return -2.0f;
    if (crc8(buf + 3, 2) != buf[5]) return -3.0f;

    int16_t raw = ((int16_t)buf[0] << 8) | buf[1];
    /* SDP810-500Pa mass flow scale factor ~ 120 (units: slm)
     * Convert raw -> L/min via scale factor. */
    return (float)raw / 120.0f;
}