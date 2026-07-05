/*
 * asic.c — NUS32toA front-end ASIC driver
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 *
 * The NUS32toA is a 64-channel SiPM readout ASIC with per-channel
 * discriminator + TDC. Three of them are used, one per detector layer.
 * Each chip is addressed at I²C addr ADDR_ASIC_BASE + chip_index.
 *
 * Register map (project-internal; matches our VHDL/RTL view of the chip
 * — the real NUS32toA has a more elaborate SPI map but this is what the
 * firmware expects; replace with the upstream datasheet map when
 * integrating a production chip).
 */
#include "asic.h"
#include "registers.h"
#include "board.h"
#include <string.h>
#include <stdio.h>

extern muon_status_t i2c_write(uint8_t addr, const uint8_t *data, uint16_t n);
extern muon_status_t i2c_read(uint8_t addr, uint8_t *data, uint16_t n);

/* Internal register cache — 64 thresholds per chip, 3 chips */
static uint8_t s_threshold[ASIC_CHIP_COUNT][ASIC_CHANNELS_PER];
static uint16_t s_tdc_window[ASIC_CHIP_COUNT];

/* NUS32toA register addresses */
#define ASIC_REG_THRESHOLD_BASE 0x10   /* 0x10..0x4F per-channel threshold */
#define ASIC_REG_TDC_WINDOW     0x50
#define ASIC_REG_GLOBAL_CFG     0x60
#define ASIC_REG_RATE_BASE      0x80   /* 64 x 16-bit rates, read-only */
#define ASIC_REG_CHIP_ID        0x00

/* ---- Helpers ---- */
static uint8_t asic_addr(uint8_t chip)
{
    return (uint8_t)(ADDR_ASIC_BASE + (chip & 0x03));
}

static muon_status_t asic_write8(uint8_t chip, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_write(asic_addr(chip), buf, 2);
}

static muon_status_t asic_read8(uint8_t chip, uint8_t reg, uint8_t *val)
{
    uint8_t buf[1] = { reg };
    MUON_RETURN_ON_ERR(i2c_write(asic_addr(chip), buf, 1));
    return i2c_read(asic_addr(chip), val, 1);
}

muon_status_t asic_init(void)
{
    memset(s_threshold, 0, sizeof(s_threshold));
    for (uint8_t c = 0; c < ASIC_CHIP_COUNT; ++c) {
        /* Global config: enable all channels, TDC clock 200 MHz, gain x1 */
        MUON_RETURN_ON_ERR(asic_write8(c, ASIC_REG_GLOBAL_CFG, 0x07));
        s_tdc_window[c] = TDC_BINS;
        uint8_t wb[3] = { ASIC_REG_TDC_WINDOW,
                          (uint8_t)(TDC_BINS >> 8),
                          (uint8_t)(TDC_BINS & 0xFF) };
        MUON_RETURN_ON_ERR(i2c_write(asic_addr(c), wb, 3));
        /* Verify chip ID */
        uint8_t id = 0;
        MUON_RETURN_ON_ERR(asic_read8(c, ASIC_REG_CHIP_ID, &id));
        if (id != 0x32) return MUON_ERR_ASIC_INIT;
    }
    return MUON_OK;
}

muon_status_t asic_set_threshold(uint8_t chip, uint8_t channel, uint8_t value)
{
    if (chip >= ASIC_CHIP_COUNT || channel >= ASIC_CHANNELS_PER)
        return MUON_ERR_INVALID_ARG;
    uint8_t reg = (uint8_t)(ASIC_REG_THRESHOLD_BASE + channel);
    MUON_RETURN_ON_ERR(asic_write8(chip, reg, value));
    s_threshold[chip][channel] = value;
    return MUON_OK;
}

muon_status_t asic_set_threshold_all(uint8_t value)
{
    for (uint8_t c = 0; c < ASIC_CHIP_COUNT; ++c) {
        /* Bulk-write all 64 thresholds in one I²C transaction */
        uint8_t buf[1 + ASIC_CHANNELS_PER];
        buf[0] = ASIC_REG_THRESHOLD_BASE;
        memset(&buf[1], value, ASIC_CHANNELS_PER);
        MUON_RETURN_ON_ERR(i2c_write(asic_addr(c), buf, sizeof(buf)));
        memset(s_threshold[c], value, ASIC_CHANNELS_PER);
    }
    return MUON_OK;
}

muon_status_t asic_set_tdc_window(uint8_t chip, uint16_t bins)
{
    if (chip >= ASIC_CHIP_COUNT) return MUON_ERR_INVALID_ARG;
    uint8_t buf[3] = { ASIC_REG_TDC_WINDOW,
                       (uint8_t)(bins >> 8),
                       (uint8_t)(bins & 0xFF) };
    MUON_RETURN_ON_ERR(i2c_write(asic_addr(chip), buf, 3));
    s_tdc_window[chip] = bins;
    return MUON_OK;
}

uint8_t asic_get_threshold(uint8_t chip, uint8_t channel)
{
    if (chip >= ASIC_CHIP_COUNT || channel >= ASIC_CHANNELS_PER) return 0;
    return s_threshold[chip][channel];
}

/* Autocalibrate: for each channel, sweep the threshold from high (insensitive)
 * down to find the noise floor (where the rate jumps). Set threshold just
 * above the noise floor to maximize sensitivity while rejecting dark counts.
 * This runs during calibration mode, taking ~30 s for all 192 channels.
 */
muon_status_t asic_autocalibrate(void)
{
    for (uint8_t c = 0; c < ASIC_CHIP_COUNT; ++c) {
        for (uint8_t ch = 0; ch < ASIC_CHANNELS_PER; ++ch) {
            uint8_t th = 250;
            uint32_t rate_prev = 0;
            uint8_t found = 0;
            while (th > 20 && !found) {
                MUON_RETURN_ON_ERR(asic_set_threshold(c, ch, th));
                muon_delay_ms(5);
                uint32_t rates[ASIC_CHANNELS_PER] = {0};
                MUON_RETURN_ON_ERR(asic_read_rates(c, rates));
                uint32_t rate = rates[ch];
                /* Noise floor: rate drops below 100 Hz AND
                 * difference from previous step is small */
                if (rate < 100 && rate_prev < 100) {
                    /* Set threshold a few counts above the noise edge */
                    uint8_t set_th = (uint8_t)(th + 5);
                    if (set_th > 255) set_th = 255;
                    MUON_RETURN_ON_ERR(asic_set_threshold(c, ch, set_th));
                    found = 1;
                }
                rate_prev = rate;
                th -= 5;
            }
            if (!found) {
                MUON_RETURN_ON_ERR(asic_set_threshold(c, ch, 100));
            }
        }
    }
    return MUON_OK;
}

muon_status_t asic_read_rates(uint8_t chip, uint32_t rates[ASIC_CHANNELS_PER])
{
    if (chip >= ASIC_CHIP_COUNT) return MUON_ERR_INVALID_ARG;
    /* 64 channels x 3 bytes each = 192 bytes; one read starting at RATE_BASE */
    uint8_t reg = ASIC_REG_RATE_BASE;
    MUON_RETURN_ON_ERR(i2c_write(asic_addr(chip), &reg, 1));
    uint8_t buf[ASIC_CHANNELS_PER * 3];
    MUON_RETURN_ON_ERR(i2c_read(asic_addr(chip), buf, sizeof(buf)));
    for (uint8_t i = 0; i < ASIC_CHANNELS_PER; ++i) {
        rates[i] = ((uint32_t)buf[i*3])
                 | ((uint32_t)buf[i*3 + 1] << 8)
                 | ((uint32_t)buf[i*3 + 2] << 16);
    }
    return MUON_OK;
}

void asic_dump_status(char *buf, uint16_t len)
{
    if (!buf || len == 0) return;
    int n = 0;
    n += snprintf(buf + n, len - n, "ASIC status (3 chips x 64 ch):\n");
    for (uint8_t c = 0; c < ASIC_CHIP_COUNT && n < len; ++c) {
        uint32_t sum = 0;
        for (uint8_t ch = 0; ch < ASIC_CHANNELS_PER; ++ch)
            sum += s_threshold[c][ch];
        n += snprintf(buf + n, len - n,
                      "  chip %u: avg_th=%lu tdc_window=%u\n",
                      c, (unsigned long)(sum / ASIC_CHANNELS_PER),
                      s_tdc_window[c]);
    }
}