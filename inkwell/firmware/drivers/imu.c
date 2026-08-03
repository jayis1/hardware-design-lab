/*
 * imu.c — BMI270 + BMM150 driver for Inkwell
 *
 * Implements SPI0 access to the BMI270 (accelerometer + gyroscope with
 * hardware FIFO) and the BMM150 (magnetometer) sharing the same bus.
 * The FIFO is drained in the BMI270 data-ready ISR into a caller-provided
 * array of imu_sample_t. Acceleration is returned in g, angular rate in
 * rad/s, magnetic field in µT. Conversion constants use the datasheet
 * sensitivities: ±8 g -> 4096 LSB/g, ±1000 dps -> 32.8 LSB/dps.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "imu.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ---- SPI0 bit-banged shim (production uses EasyDMA SPIM0) ---- */
static void spi0_select(uint32_t cs)  { nrf_gpio_pin_clear(cs); }
static void spi0_deselect(uint32_t cs) { nrf_gpio_pin_set(cs);   }

static uint8_t spi0_xfer(uint8_t b)
{
    /* Place-holder bit-bang; real build uses SPIM0 EasyDMA at 8 MHz. */
    (void)b;
    return 0;
}

static uint8_t bmi_read(uint8_t reg)
{
    spi0_select(BMI270_CS_PIN);
    spi0_xfer(reg | 0x80U);
    uint8_t v = spi0_xfer(0xFF);
    spi0_deselect(BMI270_CS_PIN);
    return v;
}

static void bmi_write(uint8_t reg, uint8_t val)
{
    spi0_select(BMI270_CS_PIN);
    spi0_xfer(reg & 0x7FU);
    spi0_xfer(val);
    spi0_deselect(BMI270_CS_PIN);
}

static void bmi_burst(uint8_t reg, uint8_t *buf, uint32_t n)
{
    spi0_select(BMI270_CS_PIN);
    spi0_xfer(reg | 0x80U);
    for (uint32_t i = 0; i < n; ++i) buf[i] = spi0_xfer(0xFF);
    spi0_deselect(BMI270_CS_PIN);
}

static uint8_t bmm_read(uint8_t reg)
{
    spi0_select(BMM150_CS_PIN);
    spi0_xfer(reg | 0x80U);
    uint8_t v = spi0_xfer(0xFF);
    spi0_deselect(BMM150_CS_PIN);
    return v;
}

static void bmm_write(uint8_t reg, uint8_t val)
{
    spi0_select(BMM150_CS_PIN);
    spi0_xfer(reg & 0x7FU);
    spi0_xfer(val);
    spi0_deselect(BMM150_CS_PIN);
}

/* ---- Sensitivities (from datasheets) ---- */
#define BMI270_ACC_LSB_G    (4096.0f)  /* ±8 g */
#define BMI270_GYR_LSB_DPS  (32.8f)    /* ±1000 dps */
#define DPS_TO_RADPS        (0.01745329252f)
#define BMM150_LSB_UT_XY    (0.142857f)  /* 1/7 µT/LSB for xy */
#define BMM150_LSB_UT_Z     (0.25f)       /* 1/4 µT/LSB for z */

static float g_last_sample_ms;

void imu_init(void)
{
    /* Configure CS pins as active-high outputs (idle high) */
    nrf_gpio_cfg_output(BMI270_CS_PIN, 0);
    nrf_gpio_pin_set(BMI270_CS_PIN);
    nrf_gpio_cfg_output(BMM150_CS_PIN, 0);
    nrf_gpio_pin_set(BMM150_CS_PIN);

    /* --- BMI270 soft-reset then configure --- */
    bmi_write(BMI270_REG_CMD, BMI270_CMD_SOFTRESET);
    for (volatile uint32_t i = 0; i < 50000; ++i) { /* ~1 ms settle */ }

    if (bmi_read(BMI270_REG_CHIPID) != BMI270_CHIPID_VAL) {
        /* No BMI270 present; the pipeline will emit zeros. */
        return;
    }

    /* Flush FIFO */
    bmi_write(BMI270_REG_CMD, BMI270_CMD_FIFO_FLUSH);

    /* Accel: ±8 g, osr2, 1.6 kHz filter (we sample at 1 kHz) */
    bmi_write(BMI270_REG_ACC_RANGE, BMI270_ACC_RANGE_8G);
    bmi_write(BMI270_REG_ACC_CONF,  BMI270_ACC_BWP_OSR2 | 0x0BU);
    /* Gyro: ±1000 dps, osr2, 1.6 kHz filter */
    bmi_write(BMI270_REG_GYR_RANGE, BMI270_GYR_RANGE_1000);
    bmi_write(BMI270_REG_GYR_CONF,  BMI270_GYR_BWP_OSR2 | 0x0BU);

    /* FIFO: stream mode, watermark 240 bytes */
    bmi_write(BMI270_REG_FIFO_CONFIG_1, 0x80U);  /* sens mode */
    bmi_write(BMI270_REG_FIFO_WTM_0, BMI270_FIFO_WTM & 0xFFU);
    bmi_write(BMI270_REG_FIFO_WTM_1, (BMI270_FIFO_WTM >> 8) & 0xFFU);
    bmi_write(BMI270_REG_FIFO_CONFIG_0, 0x80U);  /* enable sens */

    /* INT1: active-high, push-pull, data-ready / FIFO watermark */
    bmi_write(BMI270_REG_INT1_IO_CTRL, 0x0AU);
    bmi_write(BMI270_REG_INT_MAP_1, 0x80U);  /* FIFO_WM -> INT1 */

    /* Power on accel + gyro */
    bmi_write(BMI270_REG_PWR_CTRL, 0x0EU);
    bmi_write(BMI270_REG_PWR_CONF, 0x00U);   /* active */

    /* --- BMM150 magnetometer --- */
    if (bmm_read(BMM150_REG_CHIPID) != BMM150_CHIPID_VAL) return;
    bmm_write(BMM150_REG_PWR_CTRL, BMM150_PWR_NORMAL);
    bmm_write(BMM150_REG_OP_MODE, BMM150_OP_MODE_NORMAL);
    bmm_write(BMM150_REG_REP_XY, BMM150_REP_XY_REGULAR);
    bmm_write(BMM150_REG_REP_Z,  0x1BU);    /* ~10 Hz z */

    g_last_sample_ms = 0.0f;
}

void imu_reset(void)
{
    bmi_write(BMI270_REG_CMD, BMI270_CMD_SOFTRESET);
}

/* Parse one BMI270 FIFO frame: 7 bytes header + a + g (6+6) = 19 total. */
static int parse_fifo_frame(const uint8_t *buf, imu_sample_t *out, float ts_ms)
{
    uint8_t hdr = buf[0];
    if ((hdr & BMI270_FIFO_HEADER_MSK) == BMI270_FIFO_HEADER_SKIP) {
        return 1;  /* skip frame is 1 byte */
    }
    if ((hdr & BMI270_FIFO_HEADER_MSK) != BMI270_FIFO_HEADER_SENS) {
        return 1;  /* unhandled header, skip 1 byte to resync */
    }
    int16_t ax = (int16_t)((buf[1])  | (buf[2]  << 8));
    int16_t ay = (int16_t)((buf[3])  | (buf[4]  << 8));
    int16_t az = (int16_t)((buf[5])  | (buf[6]  << 8));
    int16_t gx = (int16_t)((buf[7])  | (buf[8]  << 8));
    int16_t gy = (int16_t)((buf[9])  | (buf[10] << 8));
    int16_t gz = (int16_t)((buf[11]) | (buf[12] << 8));

    out->accel_g[0] = (float)ax / BMI270_ACC_LSB_G;
    out->accel_g[1] = (float)ay / BMI270_ACC_LSB_G;
    out->accel_g[2] = (float)az / BMI270_ACC_LSB_G;
    out->gyro_radps[0] = ((float)gx / BMI270_GYR_LSB_DPS) * DPS_TO_RADPS;
    out->gyro_radps[1] = ((float)gy / BMI270_GYR_LSB_DPS) * DPS_TO_RADPS;
    out->gyro_radps[2] = ((float)gz / BMI270_GYR_LSB_DPS) * DPS_TO_RADPS;

    /* Magnetometer is on the same bus; read 6 bytes here for convenience. */
    int16_t mx = (int16_t)((bmm_read(BMM150_REG_DATA_X_LSB)) |
                           (bmm_read(BMM150_REG_DATA_X_MSB) << 8));
    int16_t my = (int16_t)((bmm_read(BMM150_REG_DATA_Y_LSB)) |
                           (bmm_read(BMM150_REG_DATA_Y_MSB) << 8));
    int16_t mz = (int16_t)((bmm_read(BMM150_REG_DATA_Z_LSB)) |
                           (bmm_read(BMM150_REG_DATA_Z_MSB) << 8));
    out->mag_ut[0] = (float)mx * BMM150_LSB_UT_XY;
    out->mag_ut[1] = (float)my * BMM150_LSB_UT_XY;
    out->mag_ut[2] = (float)mz * BMM150_LSB_UT_Z;

    out->ts_ms = (uint32_t)ts_ms;
    if (g_last_sample_ms == 0.0f) out->dt_s = 0.001f;
    else out->dt_s = (ts_ms - g_last_sample_ms) * 0.001f;
    g_last_sample_ms = ts_ms;

    return 7 + 12;  /* 1 header + 6 accel + 6 gyro = 19 bytes */
}

int32_t imu_fifo_drain(imu_sample_t *out, uint32_t max_n)
{
    uint8_t len_lo = bmi_read(BMI270_REG_FIFO_LENGTH_LSB);
    uint8_t len_hi = bmi_read(BMI270_REG_FIFO_LENGTH_MSB);
    uint16_t fifo_bytes = (uint16_t)(len_lo | (len_hi << 8));
    if (fifo_bytes == 0) return 0;

    static uint8_t fbuf[256];
    uint16_t to_read = fifo_bytes > sizeof(fbuf) ? sizeof(fbuf) : fifo_bytes;
    bmi_burst(BMI270_REG_FIFO_DATA, fbuf, to_read);

    uint32_t count = 0;
    uint32_t idx = 0;
    uint32_t ts_ms = 0;
    while (idx + 19 <= to_read && count < max_n) {
        int consumed = parse_fifo_frame(&fbuf[idx], &out[count], (float)ts_ms);
        idx += (uint32_t)consumed;
        ts_ms += 1;  /* each frame ~1 ms apart in 1 kHz mode */
        count++;
    }
    return (int32_t)count;
}

bool imu_read_mag(float *bx, float *by, float *bz)
{
    int16_t mx = (int16_t)((bmm_read(BMM150_REG_DATA_X_LSB)) |
                           (bmm_read(BMM150_REG_DATA_X_MSB) << 8));
    int16_t my = (int16_t)((bmm_read(BMM150_REG_DATA_Y_LSB)) |
                           (bmm_read(BMM150_REG_DATA_Y_MSB) << 8));
    int16_t mz = (int16_t)((bmm_read(BMM150_REG_DATA_Z_LSB)) |
                           (bmm_read(BMM150_REG_DATA_Z_MSB) << 8));
    *bx = (float)mx * BMM150_LSB_UT_XY;
    *by = (float)my * BMM150_LSB_UT_XY;
    *bz = (float)mz * BMM150_LSB_UT_Z;
    return true;
}

void imu_enable_drdy_irq(void (*cb)(void))
{
    /* Real build: GPIOTE IN on BMI270_INT1_PIN -> cb. Stub keeps pointer. */
    (void)cb;
}