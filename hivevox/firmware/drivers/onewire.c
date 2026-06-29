/*
 * drivers/onewire.c — 1-Wire driver for DS18B20 temperature probes
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Bit-banged 1-Wire on PB0 (external 4.7k pullup to 3V3).
 * Supports up to 3 DS18B20 probes on the same bus (parasitic power off;
 * we use VDD-powered probes for reliability in cold apiaries).
 */
#include "onewire.h"
#include "../registers.h"
#include "../board.h"

/* PB0 pin helpers */
#define OW_PIN  0  /* bit 0 of GPIOB */

static inline void ow_pin_output(void)
{
    GPIOB->MODER &= ~(3U << (OW_PIN*2));
    GPIOB->MODER |=  (GPIO_MODE_OUTPUT << (OW_PIN*2));
    GPIOB->OTYPER |= (1U << OW_PIN);  /* open-drain */
}

static inline void ow_pin_input(void)
{
    GPIOB->MODER &= ~(3U << (OW_PIN*2));
    GPIOB->OTYPER &= ~(1U << OW_PIN);
    GPIOB->PUPDR |= (1U << (OW_PIN*2)); /* pull-up */
}

static inline void ow_drive_low(void)
{
    GPIOB->BRR = (1U << OW_PIN);
}

static inline void ow_release(void)
{
    GPIOB->BSRR = (1U << OW_PIN);
}

static inline int ow_read_pin(void)
{
    return (GPIOB->IDR >> OW_PIN) & 1;
}

/* Microsecond delay via SysTick countdown (80 MHz core). */
static void delay_us(uint32_t us)
{
    /* Use DWT cycle counter if available; fallback to loop.
     * We assume DWT was enabled in main(). */
    extern volatile uint32_t g_dwt_cycles_per_us;
    volatile uint32_t start = dwt_cycle_count();
    uint32_t target = us * g_dwt_cycles_per_us;
    while ((dwt_cycle_count() - start) < target) { }
}

/* ---- 1-Wire timing-critical primitives ---------------------------- */

/* Reset pulse: drive low >=480µs, release, wait for presence ~70µs. */
int ow_reset(void)
{
    int presence;
    ow_pin_output();
    ow_drive_low();
    delay_us(500);
    ow_release();
    ow_pin_input();
    delay_us(70);
    presence = !ow_read_pin();  /* low = device present */
    delay_us(430);
    return presence;
}

/* Write one bit. */
static void ow_write_bit(int bit)
{
    ow_pin_output();
    if (bit) {
        ow_drive_low();  delay_us(6);
        ow_release();    delay_us(64);
    } else {
        ow_drive_low();  delay_us(60);
        ow_release();    delay_us(10);
    }
}

/* Read one bit. */
static int ow_read_bit(void)
{
    int bit;
    ow_pin_output();
    ow_drive_low();  delay_us(4);
    ow_release();    ow_pin_input();
    delay_us(8);
    bit = ow_read_pin();
    delay_us(52);
    return bit;
}

void ow_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; i++) {
        ow_write_bit(b & 1);
        b >>= 1;
    }
}

uint8_t ow_read_byte(void)
{
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) {
        b >>= 1;
        if (ow_read_bit()) b |= 0x80;
    }
    return b;
}

/* ---- DS18B20 commands ---------------------------------------------- */
#define OW_CMD_SKIP_ROM        0xCC
#define OW_CMD_CONVERT_T       0x44
#define OW_CMD_READ_SCRATCH     0xBE
#define OW_CMD_READ_ROM        0x33

/* Trigger conversion on all probes simultaneously (skip ROM). */
int ds18b20_start_all(void)
{
    if (!ow_reset()) return -1;
    ow_write_byte(OW_CMD_SKIP_ROM);
    ow_write_byte(OW_CMD_CONVERT_T);
    return 0;
}

/* Poll for conversion completion (DQ goes high when done). */
int ds18b20_wait_done(uint32_t timeout_ms)
{
    for (uint32_t i = 0; i < timeout_ms; i++) {
        if (ow_read_bit()) return 0;
        delay_ms(1);
    }
    return -1;
}

/*
 * Read temperature from all probes. Since we use skip-ROM (single bus read
 * after a broadcast convert), all probes return the same scratchpad. For
 * multi-probe deployments, each probe has a unique ROM ID; the caller should
 * use match-ROM. Here we provide a simplified 3-probe sequential read using
 * indexed device ROM IDs discovered at boot. We keep it simple: read one
 * scratchpad per call; caller iterates and re-issues convert.
 */
int ds18b20_read_scratch(int16_t *temp_centi)
{
    uint8_t scratch[9];
    if (!ow_reset()) return -1;
    ow_write_byte(OW_CMD_SKIP_ROM);
    ow_write_byte(OW_CMD_READ_SCRATCH);
    for (int i = 0; i < 9; i++) scratch[i] = ow_read_byte();

    /* CRC-8 check (poly 0x31, init 0) */
    uint8_t crc = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t b = scratch[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
            b <<= 1;
        }
    }
    if (crc != scratch[8]) return -2;

    int16_t raw = (scratch[1] << 8) | scratch[0];
    /* DS18B20 default 12-bit: raw * 0.0625 °C → centi = raw * 100 / 16 */
    *temp_centi = (int16_t)((int32_t)raw * 100 / 16);
    return 0;
}

/*
 * For the 3-probe HiveVox layout we issue convert on each individually using
 * a pre-discovered ROM address. This is a simplified placeholder: a real
 * deployment reads the 64-bit ROM of each probe at boot via SEARCH ROM and
 * caches them. Here we expose a per-probe read API using match-ROM.
 */
int ds18b20_read_by_rom(const uint8_t rom[8], int16_t *temp_centi)
{
    uint8_t scratch[9];
    if (!ow_reset()) return -1;
    /* Match ROM */
    ow_write_byte(0x55);
    for (int i = 0; i < 8; i++) ow_write_byte(rom[i]);
    ow_write_byte(OW_CMD_READ_SCRATCH);
    for (int i = 0; i < 9; i++) scratch[i] = ow_read_byte();

    uint8_t crc = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t b = scratch[i];
        for (int k = 0; k < 8; k++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
            b <<= 1;
        }
    }
    if (crc != scratch[8]) return -2;

    int16_t raw = (scratch[1] << 8) | scratch[0];
    *temp_centi = (int16_t)((int32_t)raw * 100 / 16);
    return 0;
}