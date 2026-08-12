/*
 * humid.c — SHT45 RH + T sensor driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Sensirion SHT45 on I2C1 (address 0x44).
 * Single measurement command 0x2400 (no clock stretching) -> 10 ms -> read 6 bytes (T,RH + CRC).
 */

#include "humid.h"
#include "../board.h"
#include "../registers.h"

extern int i2c_write_cmd(uint8_t addr, uint16_t cmd);
extern int i2c_read_bytes(uint8_t addr, uint8_t *buf, uint16_t n);
extern uint8_t crc8(const uint8_t *data, uint8_t len); /* from co2.c */

int humid_init(void) {
    /* Soft reset */
    if (i2c_write_cmd(SHT45_I2C_ADDR, SHT45_CMD_SOFTRESET) < 0) return -1;
    delay_ms(5);
    return 0;
}

int humid_measure(humid_meas_t *out) {
    if (i2c_write_cmd(SHT45_I2C_ADDR, SHT45_CMD_MEASURE_NOCLKSTRETCH) < 0) return -1;
    delay_ms(SHT45_MEAS_TIME_MS + 2);

    uint8_t buf[6];
    if (i2c_read_bytes(SHT45_I2C_ADDR, buf, 6) < 0) return -1;

    if (crc8(&buf[0], 2) != buf[2]) return -2;
    if (crc8(&buf[3], 2) != buf[5]) return -2;

    uint16_t t_raw  = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t rh_raw = ((uint16_t)buf[3] << 8) | buf[4];

    /* SHT45 formulas */
    out->temperature_x100 = (int16_t)((int32_t)t_raw * 17500 / 65535 - 4500);
    out->humidity_x100    = (int16_t)((int32_t)rh_raw * 12500 / 65535 - 600);
    if (out->humidity_x100 < 0)   out->humidity_x100 = 0;
    if (out->humidity_x100 > 10000) out->humidity_x100 = 10000;

    return 0;
}