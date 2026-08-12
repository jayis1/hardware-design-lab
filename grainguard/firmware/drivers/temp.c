/*
 * temp.c — DS18B20 1-Wire 9-zone temperature driver
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Drives 9 DS18B20 digital thermometers on a single 1-Wire bus.
 * Uses SKIP_ROM for simultaneous conversion, then MATCH_ROM for reads.
 * Parasitic power: a strong pull-up MOSFET (PC0) supplies current
 * during conversion.
 */

#include "temp.h"
#include "../board.h"
#include "../registers.h"

/* Discovered ROM IDs (8 bytes each) */
static uint8_t rom_ids[TEMP_NUM_ZONES][8];
static uint8_t num_found = 0;

/* ---- 1-Wire low-level ---- */
/* Bus is PA8, open-drain. 0 = drive low, 1 = release (external pull-up). */

static void ow_set_output_low(void) {
    /* Set MODER bit to output, OTYPER to push-pull temporarily for speed */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA8__ONEWIRE * 2)))
                  | (GPIO_MODE_OUTPUT << (PA8__ONEWIRE * 2));
    GPIOA->OTYPER &= ~(1 << PA8__ONEWIRE);
    GPIOA->BSRR = (1 << (PA8__ONEWIRE + 16));  /* low */
}

static void ow_release(void) {
    /* Switch back to open-drain input (external 4.7k pull-up brings high) */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA8__ONEWIRE * 2)))
                  | (GPIO_MODE_INPUT << (PA8__ONEWIRE * 2));
    /* Enable pull-up to ensure high when not driven */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0x3 << (PA8__ONEWIRE * 2)))
                  | (GPIO_PUPD_PU << (PA8__ONEWIRE * 2));
}

static int ow_read_bit(void) {
    ow_set_output_low();
    delay_ms(0);  /* ~5 us pulse; delay granularity is coarse; fine-tune with NOPs */
    ow_release();
    delay_ms(0);
    /* Read within 15 us of releasing */
    return (GPIOA->IDR >> PA8__ONEWIRE) & 1;
}

static void ow_write_bit(int bit) {
    if (bit) {
        ow_set_output_low();
        delay_ms(0);
        ow_release();
        delay_ms(0);  /* recovery */
    } else {
        ow_set_output_low();
        delay_ms(0);
        ow_release();
        delay_ms(0);
    }
}

static int ow_reset(void) {
    ow_set_output_low();
    delay_ms(1);   /* > 480 us; 1 ms is safe */
    ow_release();
    delay_ms(1);   /* wait for presence pulse */
    int presence = ((GPIOA->IDR >> PA8__ONEWIRE) & 1) ? 0 : 1; /* low = present */
    delay_ms(1);   /* finish presence window */
    return presence;
}

static void ow_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 1);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        val |= (ow_read_bit() << i);
    }
    return val;
}

/* ---- Strong pull-up for parasitic power ---- */
static void ow_strong_pullup_on(void) {
    /* Drive PC0 high -> MOSFET gate -> strong 5V pull-up on the bus */
    GPIOC->BSRR = (1 << PC0__TEMP_SUPPLY_EN);
}
static void ow_strong_pullup_off(void) {
    GPIOC->BSRR = (1 << (PC0__TEMP_SUPPLY_EN + 16));
}

/* ---- ROM search (first-pass discovery) ---- */
/* Simplified: assumes exactly 9 sensors pre-wired; we use SKIP_ROM for
 * conversion and sequential READ_ROM via SEARCH_ROM to populate IDs.
 * For brevity we use a simplified approach: read all 9 via SKIP_ROM
 * (conversion) then use SEARCH_ROM to enumerate.
 * In practice, a full search algorithm would populate rom_ids[]. */

static int ow_search_rom(uint8_t roms[][8], uint8_t max) {
    /* Full 1-Wire search algorithm. Returns number of devices found.
     * This is a simplified version; a production implementation follows
     * the Maxim AN187 binary tree search. */
    uint8_t found = 0;
    int last_zero = -1;

    if (!ow_reset()) return 0;
    ow_write_byte(DS18B20_CMD_SEARCH_ROM);

    for (uint8_t dev = 0; dev < max; dev++) {
        uint8_t rom[8] = {0};
        int ok = 1;
        for (uint8_t pos = 0; pos < 64; pos++) {
            int bit_a = ow_read_bit();
            int bit_b = ow_read_bit();
            if (bit_a == 0 && bit_b == 0) {
                /* Discrepancy: choose 0 first (or last_zero path) */
                int dir = (pos < (last_zero + 1)) ? 0 : 1;
                ow_write_bit(dir);
                if (dir) last_zero = pos;
                rom[pos / 8] |= (dir << (pos % 8));
            } else if (bit_a == 1 && bit_b == 0) {
                ow_write_bit(1);
                rom[pos / 8] |= (1 << (pos % 8));
            } else if (bit_a == 0 && bit_b == 1) {
                ow_write_bit(0);
                /* bit already 0 */
            } else {
                /* No devices */
                ok = 0;
                break;
            }
        }
        if (!ok) break;
        for (int i = 0; i < 8; i++) roms[found][i] = rom[i];
        found++;
        if (found >= max) break;
    }
    return found;
}

/* ---- CRC-8 (Dallas 1-Wire, poly 0x31 reversed = 0x8C, init 0) ---- */
static uint8_t crc8_ow(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t b = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ b) & 1;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            b >>= 1;
        }
    }
    return crc;
}

/* ---- Public API ---- */

int temp_init(void) {
    /* Configure PA8 as open-drain output initially */
    GPIOA->MODER = (GPIOA->MODER & ~(0x3 << (PA8__ONEWIRE * 2)))
                  | (GPIO_MODE_OUTPUT << (PA8__ONEWIRE * 2));
    GPIOA->OTYPER |= (1 << PA8__ONEWIRE);  /* open-drain */
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(0x3 << (PA8__ONEWIRE * 2)))
                  | (GPIO_PUPD_PU << (PA8__ONEWIRE * 2));

    /* Configure PC0 (strong pull-up MOSFET gate) as output, low */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (PC0__TEMP_SUPPLY_EN * 2)))
                  | (GPIO_MODE_OUTPUT << (PC0__TEMP_SUPPLY_EN * 2));
    ow_strong_pullup_off();

    /* Discover sensors */
    if (!ow_reset()) return -1;
    num_found = (uint8_t)ow_search_rom(rom_ids, TEMP_NUM_ZONES);
    return (num_found == TEMP_NUM_ZONES) ? 0 : -1;
}

int temp_trigger_conversion(void) {
    if (!ow_reset()) return -1;
    ow_write_byte(DS18B20_CMD_SKIP_ROM);
    ow_write_byte(DS18B20_CMD_CONVERT_T);
    ow_strong_pullup_on();           /* provide parasitic power */
    delay_ms(DS18B20_CONV_TIME_MS);  /* 750 ms at 12-bit */
    ow_strong_pullup_off();
    return 0;
}

static int read_single_zone(int idx, int16_t *out) {
    if (!ow_reset()) return -1;
    /* MATCH_ROM to select sensor */
    ow_write_byte(DS18B20_CMD_MATCH_ROM);
    for (int i = 0; i < 8; i++) ow_write_byte(rom_ids[idx][i]);
    ow_write_byte(DS18B20_CMD_READ_SCRATCH);

    /* Read 9 bytes: 8 data + 1 CRC */
    uint8_t scratch[9];
    for (int i = 0; i < 9; i++) scratch[i] = ow_read_byte();

    if (crc8_ow(scratch, 8) != scratch[8]) return -2;

    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    /* DS18B20 default 12-bit: 0.0625 °C per LSB */
    *out = (int16_t)((raw * 10) / 16);  /* ×10 for 0.1 °C resolution */
    return 0;
}

int temp_read_profile(temp_profile_t *out) {
    if (num_found == 0) return -1;
    out->valid_mask = 0;
    for (int i = 0; i < TEMP_NUM_ZONES; i++) {
        int16_t t;
        if (read_single_zone(i, &t) == 0) {
            out->celsius_x10[i] = t;
            out->valid_mask |= (1 << i);
        } else {
            out->celsius_x10[i] = -999;  /* sentinel */
        }
    }
    temp_compute_stats(out);
    return 0;
}

void temp_compute_stats(temp_profile_t *out) {
    int16_t max = -9990, min = 9990;
    out->max_zone = -1;
    out->min_zone = -1;
    for (int i = 0; i < TEMP_NUM_ZONES; i++) {
        if (!(out->valid_mask & (1 << i))) continue;
        int16_t t = out->celsius_x10[i];
        if (t > max) { max = t; out->max_zone = (int8_t)i; }
        if (t < min) { min = t; out->min_zone = (int8_t)i; }
    }
    out->delta_x10 = (out->max_zone >= 0) ? (int16_t)(max - min) : 0;
}

/* Return the raw ROM ID for diagnostics. Not declared in header. */
void temp_get_rom(int idx, uint8_t rom[8]) {
    if (idx < 0 || idx >= TEMP_NUM_ZONES) return;
    for (int i = 0; i < 8; i++) rom[i] = rom_ids[idx][i];
}