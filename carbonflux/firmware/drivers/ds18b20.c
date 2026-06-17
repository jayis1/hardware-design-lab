/**
 * @file    ds18b20.c
 * @brief   DS18B20 1-Wire temperature sensor driver for CarbonFlux.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Supports up to 3 DS18B20 sensors on a single 1-Wire bus (parasitic power).
 * Uses ROM-matched addressing for multi-probe operation at 5, 15, 30 cm depths.
 * 12-bit resolution gives ±0.5°C accuracy with 750 ms conversion time.
 */

#include "ds18b20.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* 1-Wire commands */
#define CMD_READ_ROM            0x33
#define CMD_SKIP_ROM            0xCC
#define CMD_MATCH_ROM           0x55
#define CMD_CONVERT_T           0x44
#define CMD_READ_SCRATCHPAD     0xBE
#define CMD_WRITE_SCRATCHPAD    0x4E
#define CMD_COPY_SCRATCHPAD     0x48
#define CMD_RECALL_EEPROM       0xB8
#define CMD_SEARCH_ROM          0xF0

#define DS18B20_FAMILY_CODE     0x28
#define CONVERSION_TIMEOUT_MS   800

static struct {
    uint64_t rom_codes[DS18B20_NUM_PROBES]; /* ROM IDs of found sensors */
    uint8_t  found;                         /* Number of sensors found */
    float    last_temps[DS18B20_NUM_PROBES];
} g_ds = {0};

/* === 1-Wire bit-bang timing (all delays approximate for 160 MHz CPU) === */

static inline void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < (us * 40); i++);
}

/* Drive bus LOW */
static inline void ow_low(void) {
    GPIO_RESET_PIN(GPIOC, 6);
    GPIOC->MODER = (GPIOC->MODER & ~0x00003000UL) | 0x00001000UL; /* Output */
}

/* Release bus (high-Z, pull-up keeps it high) */
static inline void ow_high(void) {
    GPIOC->MODER = (GPIOC->MODER & ~0x00003000UL) | 0x00001000UL; /* Output */
    GPIO_SET_PIN(GPIOC, 6); /* Drive high */
    /* Switch to input after driving high briefly */
    GPIOC->MODER = (GPIOC->MODER & ~0x00003000UL) | 0x00000000UL; /* Input */
}

/* Read bus state */
static inline uint8_t ow_read(void) {
    GPIOC->MODER = (GPIOC->MODER & ~0x00003000UL) | 0x00000000UL; /* Input */
    delay_us(1);
    return GPIO_READ_PIN(GPIOC, 6);
}

static uint8_t ow_reset(void) {
    ow_low();
    delay_us(480);
    ow_high();
    delay_us(70);
    uint8_t presence = (ow_read() == 0) ? 1 : 0;
    delay_us(410);
    return presence;
}

static void ow_write_bit(uint8_t bit) {
    if (bit) {
        ow_low();
        delay_us(1);
        ow_high();
        delay_us(60);
    } else {
        ow_low();
        delay_us(60);
        ow_high();
        delay_us(2);
    }
}

static uint8_t ow_read_bit(void) {
    ow_low();
    delay_us(1);
    ow_high();
    delay_us(1);
    uint8_t bit = ow_read() ? 1 : 0;
    delay_us(60);
    return bit;
}

static void ow_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        ow_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ow_read_byte(void) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ow_read_bit()) byte |= 0x80;
    }
    return byte;
}

static void ow_write_rom(uint64_t rom) {
    for (int i = 0; i < 8; i++) {
        ow_write_byte((uint8_t)(rom >> (i * 8)));
    }
}

static uint64_t ow_read_rom(void) {
    uint64_t rom = 0;
    for (int i = 0; i < 8; i++) {
        rom |= ((uint64_t)ow_read_byte() << (i * 8));
    }
    return rom;
}

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) crc = (crc << 1) ^ 0x31;
            else crc <<= 1;
        }
    }
    return crc;
}

int ds18b20_init(void) {
    memset(g_ds.rom_codes, 0, sizeof(g_ds.rom_codes));
    g_ds.found = 0;
    g_ds.last_temps[0] = g_ds.last_temps[1] = g_ds.last_temps[2] = -127.0f;

    /* Search for up to DS18B20_NUM_PROBES sensors */
    for (int attempt = 0; attempt < 3; attempt++) {
        if (ow_reset() == 0) continue;

        ow_write_byte(CMD_READ_ROM);
        uint64_t rom = ow_read_rom();
        if ((rom & 0xFF) == DS18B20_FAMILY_CODE) {
            /* Check if already found */
            bool duplicate = false;
            for (uint8_t i = 0; i < g_ds.found; i++) {
                if (g_ds.rom_codes[i] == rom) { duplicate = true; break; }
            }
            if (!duplicate && g_ds.found < DS18B20_NUM_PROBES) {
                g_ds.rom_codes[g_ds.found++] = rom;
            }
        }
    }

    /* Set all to 12-bit resolution */
    for (uint8_t i = 0; i < g_ds.found; i++) {
        if (ow_reset() == 0) continue;
        ow_write_byte(CMD_MATCH_ROM);
        ow_write_rom(g_ds.rom_codes[i]);
        ow_write_byte(CMD_WRITE_SCRATCHPAD);
        ow_write_byte(0x7F); /* TH = high alarm */
        ow_write_byte(0x00); /* TL = low alarm */
        ow_write_byte(0x7F); /* Config: 12-bit (R1=1, R0=1) */
    }

    return g_ds.found > 0 ? 0 : -ERR_SENSOR_NOT_FOUND;
}

int ds18b20_start_conversion(uint8_t index) {
    if (index >= g_ds.found) return -ERR_SENSOR_NOT_FOUND;
    if (ow_reset() == 0) return -ERR_I2C_TIMEOUT;
    ow_write_byte(CMD_MATCH_ROM);
    ow_write_rom(g_ds.rom_codes[index]);
    ow_write_byte(CMD_CONVERT_T);
    return 0;
}

void ds18b20_start_all(void) {
    if (ow_reset()) {
        ow_write_byte(CMD_SKIP_ROM);
        ow_write_byte(CMD_CONVERT_T);
    }
}

float ds18b20_read_temp(uint8_t index) {
    if (index >= g_ds.found) return -127.0f;

    /* Start conversion and wait */
    ds18b20_start_conversion(index);
    for (volatile uint32_t i = 0; i < 15000000; i++); /* ~750 ms delay */

    /* Read scratchpad */
    if (ow_reset() == 0) return g_ds.last_temps[index];
    ow_write_byte(CMD_MATCH_ROM);
    ow_write_rom(g_ds.rom_codes[index]);
    ow_write_byte(CMD_READ_SCRATCHPAD);

    uint8_t sp[9];
    for (int i = 0; i < 9; i++) {
        sp[i] = ow_read_byte();
    }

    /* Verify CRC */
    if (ds18b20_crc8(sp, 8) != sp[8]) {
        return g_ds.last_temps[index];
    }

    /* Convert raw temperature */
    int16_t raw = (int16_t)((uint16_t)sp[1] << 8 | sp[0]);
    g_ds.last_temps[index] = (float)(raw >> 4) + (float)(raw & 0x0F) * 0.0625f;
    return g_ds.last_temps[index];
}

void ds18b20_read_all(float *temps_out) {
    /* Start simultaneous conversion */
    ds18b20_start_all();
    for (volatile uint32_t i = 0; i < 15000000; i++);

    for (uint8_t i = 0; i < g_ds.found; i++) {
        temps_out[i] = ds18b20_read_temp(i);
    }
    for (uint8_t i = g_ds.found; i < DS18B20_NUM_PROBES; i++) {
        temps_out[i] = -127.0f;
    }
}

uint8_t ds18b20_get_found(void) {
    return g_ds.found;
}