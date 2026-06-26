/*
 * mems_driver.c — ICM-42688 MEMS accelerometer driver (1 kHz, SPI)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "mems_driver.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static volatile mems_frame_t g_mf_a, g_mf_b;
static volatile mems_frame_t *g_mf_active = &g_mf_a;
static volatile mems_frame_t *g_mf_ready  = NULL;
static volatile bool          g_mf_owned  = false;
static uint16_t               g_mf_idx    = 0;

/* SPI helpers (shared bus with ADS1256; CS distinguishes device) */
static void spi_sel(void)        { /* ICM42688 CS low */ }
static void spi_desel(void)      { /* ICM42688 CS high */ }
static uint8_t spi_x(uint8_t t)  { (void)t; return 0xFF; }
static void    delay_ms(uint32_t m){ (void)m; }

static uint8_t mems_read(uint8_t reg)
{
    uint8_t v;
    spi_sel();
    spi_x(ICM42688_SPI_READ(reg));
    v = spi_x(0x00);
    spi_desel();
    return v;
}

static void mems_write(uint8_t reg, uint8_t val)
{
    spi_sel();
    spi_x(ICM42688_SPI_WRITE(reg));
    spi_x(val);
    spi_desel();
}

static void mems_read_burst(uint8_t reg, uint8_t *buf, uint16_t n)
{
    uint16_t i;
    spi_sel();
    spi_x(ICM42688_SPI_READ(reg));
    for (i = 0; i < n; i++) buf[i] = spi_x(0x00);
    spi_desel();
}

void mems_init(void)
{
    /* Software reset */
    mems_write(ICM42688_REG_DEVICE_CONFIG, 0x01);
    delay_ms(2);

    /* Power: accel in low-power 1 kHz, gyro off, temp off */
    mems_write(ICM42688_REG_PWR_MGMT0, ICM42688_PWR_ACCEL_LP | ICM42688_PWR_GYRO_OFF | ICM42688_PWR_TEMP_OFF);
    delay_ms(2);

    /* Accel config: 1 kHz ODR, ±16 g FS */
    mems_write(ICM42688_REG_ACCEL_CONFIG0, ICM42688_ACCEL_ODR_1KHZ | ICM42688_ACCEL_FS_16G);

    /* FIFO: stream mode, collect accel only */
    mems_write(ICM42688_REG_FIFO_CONFIG, 0x40); /* STREAM */
    mems_enable_fifo(true);

    /* DRDY on INT1, active-low, latched */
    mems_write(ICM42688_REG_INT_CONFIG, 0x00);
    mems_write(ICM42688_REG_INT_CONFIG0, 0x00);
    mems_write(ICM42688_REG_INT_SOURCE0, 0x08); /* UI_DRDY_INT1_EN */

    g_mf_idx = 0;
    g_mf_active = &g_mf_a;
    g_mf_ready  = NULL;
    g_mf_owned  = false;
}

void mems_set_odr(uint8_t odr)
{
    uint8_t cur = mems_read(ICM42688_REG_ACCEL_CONFIG0);
    cur = (cur & 0xF0) | (odr & 0x0F);
    mems_write(ICM42688_REG_ACCEL_CONFIG0, cur);
}

void mems_set_fs(uint8_t fs)
{
    uint8_t cur = mems_read(ICM42688_REG_ACCEL_CONFIG0);
    cur = (cur & 0x0F) | (fs & 0x70);
    mems_write(ICM42688_REG_ACCEL_CONFIG0, cur);
}

void mems_enable_fifo(bool on)
{
    /* FIFO_CONFIG bit6 = FIFO_MODE stream; bit0-5 = watermark */
    mems_write(ICM42688_REG_FIFO_CONFIG, on ? 0x40 : 0x00);
}

uint8_t mems_whoami(void)
{
    return mems_read(ICM42688_REG_WHO_AM_I);
}

float mems_read_temp_c(void)
{
    int8_t  hi = (int8_t)mems_read(ICM42688_REG_TEMP_DATA1);
    uint8_t lo = mems_read(ICM42688_REG_TEMP_DATA0);
    float t = (float)hi + ((float)lo / 256.0f);
    return t;   /* ICM-42688: T_celsius = raw (already scaled) */
}

void mems_drdy_isr(void)
{
    uint8_t buf[6];
    int16_t x, y, z;
    uint32_t ts = 0; /* disciplined µs counter in real build */

    mems_read_burst(ICM42688_REG_ACCEL_DATA_X1, buf, 6);
    x = ((int16_t)buf[0] << 8) | buf[1];
    y = ((int16_t)buf[2] << 8) | buf[3];
    z = ((int16_t)buf[4] << 8) | buf[5];

    g_mf_active->s[g_mf_idx].x = x;
    g_mf_active->s[g_mf_idx].y = y;
    g_mf_active->s[g_mf_idx].z = z;
    g_mf_active->s[g_mf_idx].ts_us = ts;
    g_mf_idx++;
    if (g_mf_idx >= MEMS_FRAME_LEN) {
        g_mf_active->len   = MEMS_FRAME_LEN;
        g_mf_active->ready = true;
        g_mf_ready         = g_mf_active;
        g_mf_owned         = true;
        g_mf_active        = (g_mf_active == &g_mf_a) ? &g_mf_b : &g_mf_a;
        g_mf_idx           = 0;
    }
}

mems_frame_t *mems_take_frame(void)
{
    if (g_mf_owned && g_mf_ready && g_mf_ready->ready) {
        return (mems_frame_t *)g_mf_ready;
    }
    return NULL;
}

void mems_release_frame(void)
{
    if (g_mf_ready) g_mf_ready->ready = false;
    g_mf_owned = false;
}