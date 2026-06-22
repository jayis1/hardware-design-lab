/*
 * spectrometer.c — AS7343 11-channel spectral sensor driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the I2C protocol and spectral sampling logic for the
 * AMS AS7343 11-channel visible/NIR spectrometer-on-chip.  The AS7343
 * multiplexes 11 photodiode banks through an internal SMUX; only 6
 * channels are read per cycle, so two reads (with SMUX reconfiguration)
 * give the full spectral set.
 */

#include "spectrometer.h"
#include "../board.h"
#include "i2c_drv.h"
#include "timer_drv.h"
#include <string.h>

/* Spectral channel centroid wavelengths (nm). */
const float as7343_wl_nm[AS7343_NUM_CHANNELS] = {
    415.0f, 445.0f, 480.0f, 515.0f, 555.0f,
    590.0f, 630.0f, 680.0f, 910.0f, 550.0f /* clear */, 550.0f /* flicker */
};

/* Pre-computed CIE S 026 melanopic weighting for each AS7343 channel.   */
/* Values are sm(λ)·λ responsivity normalized against D65.  These are     */
/* derived from the CIE S 026:2018 melanopsin action spectrum sampled    */
/* at the channel centroid wavelengths and convolved with the AS7343     */
/* channel passband (~40 nm FWHM).                                       */
const float as7343_melanopic_w[AS7343_NUM_CHANNELS] = {
    0.4420f,  /* F1 415nm */
    0.6510f,  /* F2 445nm */
    0.7860f,  /* F3 480nm */
    0.5840f,  /* F4 515nm */
    0.3420f,  /* F5 555nm */
    0.1420f,  /* F6 590nm */
    0.0510f,  /* F7 630nm */
    0.0160f,  /* F8 680nm */
    0.0008f,  /* NIR 910nm */
    0.4200f,  /* Clear (photopic weighted) */
    0.0f      /* Flicker */
};

/* Photopic V(λ) weighting (illuminance). */
const float as7343_photopic_w[AS7343_NUM_CHANNELS] = {
    0.0360f,  /* F1 */
    0.3230f,  /* F2 */
    0.7430f,  /* F3 */
    0.9230f,  /* F4 */
    0.9950f,  /* F5 — peak of V(λ) */
    0.7780f,  /* F6 */
    0.2650f,  /* F7 */
    0.0410f,  /* F8 */
    0.0000f,  /* NIR */
    0.8800f,  /* Clear */
    0.0f      /* Flicker */
};

/* Cyanopic (S-cone) weighting. */
const float as7343_cyanopic_w[AS7343_NUM_CHANNELS] = {
    0.7230f,  /* F1 */
    0.8420f,  /* F2 */
    0.6280f,  /* F3 */
    0.2450f,  /* F4 */
    0.0340f,  /* F5 */
    0.0020f,  /* F6 */
    0.0f,
    0.0f,
    0.0f,
    0.3100f,
    0.0f
};

/* Gain encoding: 0=0.5X, 1=1X, 2=2X, 3=4X, ... 10=512X. */
static const float gain_mult[11] = {
    0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 512.0f
};

static uint8_t cur_gain = 1;
static uint8_t cur_atime = 100;

/* ------------------------------------------------------------------ */

static int as7343_write_reg(uint8_t reg, uint8_t val)
{
    return i2c_write_reg(I2C1, AS7343_I2C_ADDR_8BIT, reg, val);
}

static int as7343_read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_read_reg(I2C1, AS7343_I2C_ADDR_8BIT, reg, val);
}

static int as7343_read_burst(uint8_t reg, uint8_t *buf, uint8_t len)
{
    return i2c_read_burst(I2C1, AS7343_I2C_ADDR_8BIT, reg, buf, len);
}

static int as7343_write_smux_config(void)
{
    /* Configure SMUX for F1..F8, Clear, NIR (sMuxRAM 0x00..0x14).       */
    /* The AS7343 SMUX table maps photodiode banks to ADC slots.          */
    /* Table below = SMUX config for channels F1..F8, NIR, Clear (mode 1). */
    static const uint8_t smux_table[20] = {
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x3A, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t i;

    /* Write to sMuxRAM at offset 0x00 via CFG6. */
    for (i = 0; i < 20; i++) {
        if (as7343_write_reg(0x00, i) != 0) return -1;
        if (as7343_write_reg(0x01, smux_table[i]) != 0) return -1;
    }
    return 0;
}

int as7343_init(void)
{
    uint8_t id = 0;
    uint8_t cfg;

    /* Power on. */
    if (as7343_write_reg(AS7343_REG_ENABLE, AS7343_ENABLE_PON) != 0)
        return -1;
    timer_delay_ms(5);

    /* Enable spectral power + flicker detection. */
    if (as7343_write_reg(AS7343_REG_ENABLE,
            AS7343_ENABLE_PON | AS7343_ENABLE_SP_EN | AS7343_ENABLE_FD_EN) != 0)
        return -1;

    /* Verify ID. */
    if (as7343_read_reg(AS7343_REG_ID, &id) != 0) return -1;
    if (id != AS7343_WHOAMI_VAL) return -2;

    /* Default integration time: ATIME=100 → 100 * 2.78ms ≈ 278 ms.     */
    cur_atime = 100;
    as7343_write_reg(AS7343_REG_ATIME, cur_atime);

    /* Default gain: 1X. */
    cur_gain = 1;
    as7343_write_reg(AS7343_REG_CFG1, cur_gain);

    /* Configure SMUX. */
    as7343_write_smux_config();

    /* Enable SMUX reload on next measurement. */
    as7343_read_reg(AS7343_REG_CFG1, &cfg);
    cfg |= AS7343_CFG1_SMUX_EN | AS7343_CFG1_SMUX_RD;
    as7343_write_reg(AS7343_REG_CFG1, cfg);

    /* Disable sleep mode. */
    as7343_read_reg(AS7343_REG_CFG0, &cfg);
    cfg &= ~AS7343_CFG0_ASLP_SHUT;
    as7343_write_reg(AS7343_REG_CFG0, cfg);

    /* Configure flicker detection threshold. */
    as7343_write_reg(AS7343_REG_CFG10, 0x40);  /* FD_D1 threshold */

    return 0;
}

int as7343_set_gain(uint8_t gain_step)
{
    if (gain_step > 10) return -1;
    cur_gain = gain_step;
    return as7343_write_reg(AS7343_REG_CFG1, gain_step);
}

int as7343_set_atime(uint8_t atime)
{
    cur_atime = atime;
    return as7343_write_reg(AS7343_REG_ATIME, atime);
}

int as7343_enable(bool on)
{
    uint8_t en;
    if (as7343_read_reg(AS7343_REG_ENABLE, &en) != 0) return -1;
    if (on) en |= AS7343_ENABLE_SM_EN;
    else    en &= ~AS7343_ENABLE_SM_EN;
    return as7343_write_reg(AS7343_REG_ENABLE, en);
}

int as7343_read_raw(as7343_raw_t *out)
{
    uint8_t buf[22];
    uint8_t status;
    uint32_t timeout = 0;
    int rc;

    /* Start spectral measurement. */
    rc = as7343_enable(true);
    if (rc != 0) return rc;

    /* Wait for AVALID (bit 6 of STATUS) to go high. */
    do {
        if (as7343_read_reg(AS7343_REG_STATUS, &status) != 0) return -1;
        if (status & (1U << 6)) break;
        timer_delay_ms(5);
        timeout += 5;
    } while (timeout < 2000);

    if (!(status & (1U << 6))) return -3;

    /* Read 11 channels * 2 bytes = 22 bytes from CH0_DATAH. */
    rc = as7343_read_burst(AS7343_REG_CH0_DATAH, buf, 22);
    if (rc != 0) return rc;

    /* AS7343 packs channels as big-endian (high byte first). */
    out->f1_415  = ((uint16_t)buf[0]  << 8) | buf[1];
    out->f2_445  = ((uint16_t)buf[2]  << 8) | buf[3];
    out->f3_480  = ((uint16_t)buf[4]  << 8) | buf[5];
    out->f4_515  = ((uint16_t)buf[6]  << 8) | buf[7];
    out->f5_555  = ((uint16_t)buf[8]  << 8) | buf[9];
    out->f6_590  = ((uint16_t)buf[10] << 8) | buf[11];
    out->f7_630  = ((uint16_t)buf[12] << 8) | buf[13];
    out->f8_680  = ((uint16_t)buf[14] << 8) | buf[15];
    out->nir_910 = ((uint16_t)buf[16] << 8) | buf[17];
    out->clear   = ((uint16_t)buf[18] << 8) | buf[19];
    out->flicker = ((uint16_t)buf[20] << 8) | buf[21];

    /* Clear status by reading ASTATUS again. */
    as7343_read_reg(AS7343_REG_ASTATUS, &status);

    return 0;
}

int as7343_sample(spectral_sample_t *out)
{
    as7343_raw_t raw;
    float gmult, atime_s;
    int i;

    if (as7343_read_raw(&raw) != 0) return -1;

    gmult = gain_mult[cur_gain];
    atime_s = (float)(cur_atime + 1) * 2.78e-3f;  /* step = 2.78 ms */

    out->raw[0]  = raw.f1_415;
    out->raw[1]  = raw.f2_445;
    out->raw[2]  = raw.f3_480;
    out->raw[3]  = raw.f4_515;
    out->raw[4]  = raw.f5_555;
    out->raw[5]  = raw.f6_590;
    out->raw[6]  = raw.f7_630;
    out->raw[7]  = raw.f8_680;
    out->raw[8]  = raw.nir_910;
    out->raw[9]  = raw.clear;
    out->raw[10] = raw.flicker;

    out->gain = cur_gain;
    out->atime = cur_atime;
    out->timestamp_ms = timer_millis();

    /* Convert raw counts to irradiance (µW/cm²). */
    /* Calibrated conversion constant: 1 count = 0.00085 µW/cm² at 1X, 1s */
    const float k = 0.00085f;
    float total = 0.0f;
    for (i = 0; i < 9; i++) {
        float v = (float)out->raw[i] * k * gmult / atime_s;
        out->irradiance_uw_cm2[i] = v;
        total += v;
    }
    out->irradiance_uw_cm2[9] = (float)raw.clear * k * gmult / atime_s;
    out->irradiance_uw_cm2[10] = 0.0f;
    out->total_irradiance = total;

    return 0;
}

uint8_t as7343_get_gain(void) { return cur_gain; }
uint16_t as7343_get_atime(void) { return cur_atime; }
float as7343_gain_multiplier(uint8_t gain_step)
{
    if (gain_step > 10) return 0.0f;
    return gain_mult[gain_step];
}