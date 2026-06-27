/*
 * env_drv.c — Environmental sensors driver (SHT40 + LPS22HB)
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * I²C driver for SHT40 (temperature/humidity) and LPS22HB (barometric
 * pressure). Used for skin microclimate compensation and altitude context.
 */

#include "env_drv.h"
#include "../board.h"
#include "../registers.h"
#include <math.h>

static env_data_t last_reading = {0};
static float last_pressure_ref = 0.0f;  /* Reference for altitude calc */

/* ---- I²C helpers ---- */
static void i2c_write_cmd(uint8_t addr, uint8_t cmd)
{
    /* TWIM: START → addr(W) → cmd → STOP */
    (void)addr; (void)cmd;
}

static void i2c_write_cmd16(uint8_t addr, uint16_t cmd)
{
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    (void)addr; (void)buf;
}

static uint16_t i2c_read16(uint8_t addr)
{
    (void)addr;
    return 0;
}

static void i2c_read_buf(uint8_t addr, uint8_t *buf, uint16_t len)
{
    (void)addr; (void)buf; (void)len;
}

/* CRC-8 for SHT40 (poly=0x31, init=0xFF) */
static uint8_t sht40_crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int env_init(void)
{
    /* Initialize SHT40: soft reset */
    i2c_write_cmd(SHT40_ADDR, SHT40_CMD_SOFTRESET);
    /* board_delay_ms(1); */

    /* Initialize LPS22HB: check WHO_AM_I */
    /* uint8_t whoami = i2c_read_reg(LPS22HB_ADDR, LPS22_REG_WHO_AM_I); */
    /* if (whoami != LPS22_WHO_AM_I_VAL) return -1; */

    /* Configure LPS22HB: ODR=10Hz, low-pass filter, BDU, active mode */
    /* i2c_write_reg(LPS22HB_ADDR, LPS22_REG_CTRL_REG1,
     *   LPS22_CTRL1_ODR_10HZ | LPS22_CTRL1_EN_LPFP | LPS22_CTRL1_BDU |
     *   LPS22_CTRL1_PD_EN); */

    /* Set initial reference pressure for relative altitude */
    last_pressure_ref = 1013.25f;  /* Standard sea level hPa */

    return 0;
}

void env_read_all(env_data_t *data)
{
    uint8_t raw[6];

    /* ---- Read SHT40 (T + RH) ---- */
    i2c_write_cmd(SHT40_ADDR, SHT40_CMD_MEAS_HIRES);
    /* board_delay_ms(SHT40_MEAS_DELAY_MS); */  /* 8 ms for high-res */
    i2c_read_buf(SHT40_ADDR, raw, 6);

    /* Parse temperature (bytes 0-1) and humidity (bytes 3-4) */
    uint16_t t_raw = (uint16_t)((raw[0] << 8) | raw[1]);
    uint16_t rh_raw = (uint16_t)((raw[3] << 8) | raw[4]);

    /* CRC check (optional) */
    /* uint8_t t_crc = sht40_crc8(raw, 2); */
    /* uint8_t rh_crc = sht40_crc8(&raw[3], 2); */

    /* Convert to physical units:
     * T = -45 + 175 * (t_raw / 2^16) [°C]
     * RH = -6 + 125 * (rh_raw / 2^16) [%RH]
     */
    float temp = SHT40_T_OFFSET + SHT40_T_COEFF * (float)t_raw / 65536.0f;
    float rh = SHT40_RH_OFFSET + SHT40_RH_COEFF * (float)rh_raw / 65536.0f;
    if (rh > 100.0f) rh = 100.0f;
    if (rh < 0.0f) rh = 0.0f;

    data->temperature_c = temp;
    data->humidity_pct = rh;

    /* ---- Read LPS22HB (pressure) ---- */
    /* i2c_write_reg(LPS22HB_ADDR, LPS22_REG_PRESS_OUT_XL | 0x80); */ /* auto-increment */
    /* i2c_read_buf(LPS22HB_ADDR, raw, 3); */ /* 24-bit pressure */

    /* For simulation, use standard pressure */
    float pressure = 1013.25f;
    /* int32_t p_raw = (raw[2] << 16) | (raw[1] << 8) | raw[0]; */
    /* pressure = (float)p_raw / 4096.0f; */  /* hPa */

    data->pressure_hpa = pressure;

    /* Calculate altitude from pressure (barometric formula) */
    /* h = 44330 * (1 - (P/P0)^(1/5.255)) */
    if (last_pressure_ref > 0.0f) {
        float ratio = pressure / last_pressure_ref;
        data->altitude_m = 44330.0f * (1.0f - powf(ratio, 0.1903f));
    } else {
        data->altitude_m = 0.0f;
    }

    /* Update last_reading cache */
    last_reading = *data;
}

float env_get_temperature(void) { return last_reading.temperature_c; }
float env_get_humidity(void)   { return last_reading.humidity_pct; }
float env_get_pressure(void)   { return last_reading.pressure_hpa; }
float env_get_altitude(void)   { return last_reading.altitude_m; }

uint8_t env_get_sht40_serial(uint32_t *serial)
{
    /* Read serial number from SHT40 */
    i2c_write_cmd(SHT40_ADDR, SHT40_CMD_READ_SN);
    uint8_t buf[6];
    i2c_read_buf(SHT40_ADDR, buf, 6);

    /* Serial is 32-bit, stored in bytes 0-3 (with CRCs in 2 and 5) */
    *serial = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
              ((uint32_t)buf[3] << 8) | buf[4];
    return 0;
}

/* ---- End of env_drv.c ---- */