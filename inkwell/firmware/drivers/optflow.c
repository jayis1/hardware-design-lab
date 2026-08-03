/*
 * optflow.c — PMW3360 optical flow driver for Inkwell drift correction
 *
 * The PMW3360 is a gaming-grade optical flow chip with a 12,000 CPI image
 * sensor and a built-in DSP that tracks surface texture. We mount it
 * downward ~3 mm above the writing plane. Every 10 ms we issue a Motion
 * Burst read, which atomically returns motion, surface quality (SQUAL),
 * shutter, raw-data sum, and maximum/minimum raw values. SQUAL > 60
 * indicates trustworthy texture; below that the pen is on glass / in air
 * and we ignore the optical flow.
 *
 * The CPI (counts-per-inch) is converted to micrometers/count:
 *   µm/count = 25400 / CPI  (since 1 inch = 25.4 mm = 25400 µm)
 * At the default 1200 CPI this is ~21.2 µm/count, fine enough for drift
 * correction on paper.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "optflow.h"
#include "../board.h"
#include "../registers.h"

/* ---- SPI1 bit-bang shim ---- */
static void spi1_select(void)   { nrf_gpio_pin_clear(PMW3360_CS_PIN); }
static void spi1_deselect(void) { nrf_gpio_pin_set(PMW3360_CS_PIN);   }
static uint8_t spi1_xfer(uint8_t b) { (void)b; return 0; }

static uint8_t pmw_read(uint8_t reg)
{
    spi1_select();
    spi1_xfer(reg & 0x7FU);
    for (volatile uint32_t i = 0; i < 50; ++i) {}  /* t_SRAD = 35 µs */
    uint8_t v = spi1_xfer(0xFF);
    spi1_deselect();
    return v;
}

static void pmw_write(uint8_t reg, uint8_t val)
{
    spi1_select();
    spi1_xfer(reg | 0x80U);
    for (volatile uint32_t i = 0; i < 2; ++i) {}
    spi1_xfer(val);
    spi1_deselect();
    for (volatile uint32_t i = 0; i < 200; ++i) {}  /* t_SAW = 1.8 ms */
}

static void pmw_burst(uint8_t reg, uint8_t *buf, uint32_t n)
{
    spi1_select();
    spi1_xfer(reg & 0x7FU);
    for (volatile uint32_t i = 0; i < 50; ++i) {}
    for (uint32_t i = 0; i < n; ++i) buf[i] = spi1_xfer(0xFF);
    spi1_deselect();
}

static uint16_t g_cpi = 1200;
static float    g_um_per_count = 25400.0f / 1200.0f;

void optflow_init(void)
{
    nrf_gpio_cfg_output(PMW3360_CS_PIN, 0);
    nrf_gpio_pin_set(PMW3360_CS_PIN);   /* idle high */
    nrf_gpio_cfg_input(PMW3360_MOTION_PIN, 0);

    /* Power-up sequence per datasheet §7.1 */
    pmw_write(PMW3360_REG_POWER_UP, 0x5AU);
    for (volatile uint32_t i = 0; i < 50000; ++i) {}
    if (pmw_read(PMW3360_REG_PRODUCT_ID) != PMW3360_PRODUCT_ID_VAL) return;

    /* CPI = config1 << 8 | (config2 & 0x0F), divided by 1200 per step. */
    pmw_write(PMW3360_REG_CONFIG1, 0x00U);  /* 1200 CPI step */
    pmw_write(PMW3360_REG_CONFIG2, 0x00U);
    pmw_write(PMW3360_REG_ANGLE_SNAP, 0x00U);   /* angle snap off */
    pmw_write(PMW3360_REG_LIFT_CONTROL, 0x00U); /* no lift cutoff */

    optflow_set_cpi(1200);
}

void optflow_set_cpi(uint16_t cpi)
{
    g_cpi = cpi;
    g_um_per_count = 25400.0f / (float)cpi;
}

void optflow_power_down(void)
{
    pmw_write(PMW3360_REG_POWER_DOWN, 0x5AU);
}

bool optflow_read(optflow_sample_t *out)
{
    /* Motion burst: motion, dx_l, dx_h, dy_l, dy_h, squal, rawsum,
       rawmax, rawmin, shutter_l, shutter_h  = 11 bytes. */
    uint8_t b[11];
    pmw_burst(PMW3360_REG_MOTION_BURST, b, sizeof(b));
    if ((b[0] & PMW3360_MOTION_MOT_BIT) == 0) {
        out->dx_counts = 0;
        out->dy_counts = 0;
        out->squal = b[5];
        out->shutter = (uint16_t)(b[9] | (b[10] << 8));
        return true;
    }
    out->dx_counts = (int16_t)((b[1]) | (b[2] << 8));
    out->dy_counts = (int16_t)((b[3]) | (b[4] << 8));
    out->squal = b[5];
    out->shutter = (uint16_t)(b[9] | (b[10] << 8));
    return true;
}

/* Exposed for dead_reckon.c via a small accessor. */
float optflow_um_per_count(void) { return g_um_per_count; }
uint16_t optflow_get_cpi(void)   { return g_cpi; }