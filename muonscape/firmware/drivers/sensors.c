/*
 * sensors.c — BME280 + LSM6DSO + LTR-329 environmental sensors
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#include "sensors.h"
#include "i2c.h"
#include "board.h"
#include "registers.h"
#include <math.h>
#include <string.h>

static env_state_t s_env;

muon_status_t sensors_init(void)
{
    memset(&s_env, 0, sizeof(s_env));
    uint8_t v;
    /* BME280 soft-reset + config */
    if (i2c_read_reg(ADDR_BME280, 0xD0, &v, 1) != MUON_OK || v != 0x60)
        return MUON_ERR_I2C;
    i2c_write_reg(ADDR_BME280, 0xE0, 0xB6);  /* reset */
    muon_delay_ms(10);
    /* ctrl_hum = x1, ctrl_meas = T:x1 P:x1 mode normal, config = 1000ms */
    i2c_write_reg(ADDR_BME280, 0xF2, 0x01);
    i2c_write_reg(ADDR_BME280, 0xF4, 0x27);
    i2c_write_reg(ADDR_BME280, 0xF5, 0xA0);

    /* LSM6DSO: CTRL1_XL = 416Hz ±2g, CTRL2_G = 416Hz ±2000dps */
    if (i2c_read_reg(ADDR_LSM6DSO, 0x0F, &v, 1) != MUON_OK || v != 0x6C)
        return MUON_ERR_I2C;
    i2c_write_reg(ADDR_LSM6DSO, 0x10, 0x6C);  /* CTRL1_XL */
    i2c_write_reg(ADDR_LSM6DSO, 0x11, 0x6C);  /* CTRL2_G */
    i2c_write_reg(ADDR_LSM6DSO, 0x12, 0x44);  /* CTRL3_C: BDU + IF_INC */

    /* LTR-329: ALS_CONTR = 0x01 (active) */
    i2c_write_reg(ADDR_LTR329, 0x80, 0x01);
    return MUON_OK;
}

static float bme280_compensate_temp(int32_t raw, int32_t *t_fine)
{
    /* Full Bosch compensation omitted for brevity; use simplified linear. */
    *t_fine = raw;
    return raw / 100.0f;  /* placeholder linear — replace with full coeff table */
}

muon_status_t sensors_read(env_state_t *st)
{
    if (!st) return MUON_ERR_INVALID_ARG;
    /* BME280: read 8 bytes starting at 0xF7 */
    uint8_t b[8];
    if (i2c_read_reg(ADDR_BME280, 0xF7, b, 8) == MUON_OK) {
        int32_t t_raw = (b[3] << 12) | (b[4] << 4) | (b[5] >> 4);
        int32_t p_raw = (b[0] << 12) | (b[1] << 4) | (b[2] >> 4);
        int32_t h_raw = (b[6] << 8) | b[7];
        int32_t tf;
        s_env.temp_c = bme280_compensate_temp(t_raw, &tf);
        s_env.pressure_hpa = p_raw / 256.0f / 100.0f;
        s_env.humidity_pct = h_raw / 1024.0f * 100.0f;
    }

    /* LSM6DSO: OUTX_L_G (0x22) 6 bytes gyro, OUTX_L_A (0x28) 6 bytes accel */
    uint8_t g[6], a[6];
    if (i2c_read_reg(ADDR_LSM6DSO, 0x22, g, 6) == MUON_OK
        && i2c_read_reg(ADDR_LSM6DSO, 0x28, a, 6) == MUON_OK) {
        int16_t gx = (g[1] << 8) | g[0];
        int16_t gy = (g[3] << 8) | g[2];
        int16_t gz = (g[5] << 8) | g[4];
        int16_t ax = (a[1] << 8) | a[0];
        int16_t ay = (a[3] << 8) | a[2];
        int16_t az = (a[5] << 8) | a[4];
        /* ±2g range: 1 LSB = 0.061 mg/LSB -> g = raw*0.061/1000 */
        s_env.accel_g[0] = ax * 0.061f / 1000.0f;
        s_env.accel_g[1] = ay * 0.061f / 1000.0f;
        s_env.accel_g[2] = az * 0.061f / 1000.0f;
        /* ±2000 dps: 70 mdps/LSB */
        s_env.gyro_dps[0] = gx * 70.0f / 1000.0f;
        s_env.gyro_dps[1] = gy * 70.0f / 1000.0f;
        s_env.gyro_dps[2] = gz * 70.0f / 1000.0f;
        /* Roll/pitch from accelerometer */
        s_env.roll_deg  = atan2f(s_env.accel_g[1], s_env.accel_g[2]) * 180.0f / 3.14159265f;
        float pitch_atan = atanf(-s_env.accel_g[0] /
            sqrtf(s_env.accel_g[1]*s_env.accel_g[1] + s_env.accel_g[2]*s_env.accel_g[2]));
        s_env.pitch_deg = pitch_atan * 180.0f / 3.14159265f;
    }

    /* LTR-329: ALS_DATA_CH1_0 (0x88) 2 bytes, CH0 (0x8A) 2 bytes */
    uint8_t l[4];
    if (i2c_read_reg(ADDR_LTR329, 0x88, l, 4) == MUON_OK) {
        uint16_t ch1 = (l[1] << 8) | l[0];
        uint16_t ch0 = (l[3] << 8) | l[2];
        /* Simplified lux calc */
        float ratio = (ch0 + ch1) > 0 ? (float)ch1 / (ch0 + ch1) : 0;
        float lux_f;
        if (ratio < 0.45f)      lux_f = (1.7743f * ch0 + 1.1059f * ch1) / 100.0f;
        else if (ratio < 0.64f) lux_f = (4.2785f * ch0 - 1.9598f * ch1) / 100.0f;
        else if (ratio < 0.85f) lux_f = (0.5926f * ch0 + 0.0850f * ch1) / 100.0f;
        else                    lux_f = 0.0f;
        s_env.lux = (uint16_t)lux_f;
    }

    *st = s_env;
    return MUON_OK;
}