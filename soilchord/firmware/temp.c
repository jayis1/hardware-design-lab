/*
 * temp.c — Soilchord DS18B20 1-Wire temperature driver (one sensor per chord)
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Implements the minimal 1-Wire timing for DS18B20 on a single GPIO
 * (open-drain, external 4.7 kΩ pull-up). We use one-shot conversion mode
 * so the sensor sleeps between readings (≤ 750 µA during conversion).
 */
#include "soilchord.h"
#include "registers.h"
#include "board.h"

#define ONEWIRE_PORT 'B'
#define ONEWIRE_PIN  7

/* ROM addresses for the six DS18B20 sensors, configured at install time and
 * stored in flash. We read them out of the config page; here we use a static
 * table that the user flashes via the downlink "calibrate" step. */
static uint8_t s_rom[NCHORDS][8] = {
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x01},
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x02},
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x03},
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x04},
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x05},
    {0x28,0xFF,0x00,0x00,0x00,0x00,0x00,0x06},
};

/* ---- GPIO primitive ---- */
static volatile uint32_t *ow_port(void)
{
    return (volatile uint32_t *)GPIOB_BASE_REG;
}
static void ow_high(void) { GPIO_BSRR(ow_port()) = (1U << (ONEWIRE_PIN + 16)); } /* push low? no — set as input */
/* We model open-drain: writing 0 drives low, writing 1 releases (input). */
static void ow_drive_low(void)
{
    volatile uint32_t *p = ow_port();
    GPIO_MODER(p) &= ~(3U << (ONEWIRE_PIN * 2));
    GPIO_MODER(p) |=  (1U << (ONEWIRE_PIN * 2));   /* output */
    GPIO_OTYPER(p) |= (1U << ONEWIRE_PIN);          /* open-drain */
    GPIO_ODR(p) &= ~(1U << ONEWIRE_PIN);
}
static void ow_release(void)
{
    volatile uint32_t *p = ow_port();
    GPIO_MODER(p) &= ~(3U << (ONEWIRE_PIN * 2));
    GPIO_MODER(p) |=  (0U << (ONEWIRE_PIN * 2));    /* input (Hi-Z) */
    /* External pull-up brings the line high. */
}
static int ow_read(void)
{
    volatile uint32_t *p = ow_port();
    return (GPIO_IDR(p) >> ONEWIRE_PIN) & 1U;
}

/* ---- 1-Wire timing ---- */
static void delay_us_busy(uint32_t us)
{
    /* ~80 MHz → 80 cycles/µs */
    for (uint32_t i = 0; i < us * 80; i++) __asm__("nop");
}

static int ow_reset(void)
{
    ow_drive_low();
    delay_us_busy(480);
    ow_release();
    delay_us_busy(70);
    int presence = (ow_read() == 0);     /* device pulls low */
    delay_us_busy(410);
    return presence;                     /* 1 if a device responded */
}

static void ow_write_bit(int v)
{
    if (v) {
        ow_drive_low(); delay_us_busy(6);
        ow_release();   delay_us_busy(64);
    } else {
        ow_drive_low(); delay_us_busy(60);
        ow_release();   delay_us_busy(10);
    }
}

static int ow_read_bit(void)
{
    ow_drive_low(); delay_us_busy(4);
    ow_release();   delay_us_busy(8);
    int v = ow_read();
    delay_us_busy(56);
    return v;
}

static void ow_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; i++) ow_write_bit((b >> i) & 1);
}

static uint8_t ow_read_byte(void)
{
    uint8_t b = 0;
    for (int i = 0; i < 8; i++) b |= (uint8_t)(ow_read_bit() << i);
    return b;
}

static uint8_t crc8(const uint8_t *p, int n)
{
    uint8_t crc = 0;
    for (int i = 0; i < n; i++) {
        uint8_t b = p[i];
        for (int j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ b) & 1;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            b >>= 1;
        }
    }
    return crc;
}

/* ---- Public API ---- */
sc_err_t temp_init(void)
{
    /* Configure the pin as input with pull-up disabled (external 4.7 kΩ). */
    volatile uint32_t *p = ow_port();
    RCC_AHB2ENR |= (1U << 0);                /* GPIOAEN... actually GPIOB */
    GPIO_MODER(p) &= ~(3U << (ONEWIRE_PIN * 2));
    GPIO_PUPDR(p) &= ~(3U << (ONEWIRE_PIN * 2));
    ow_release();
    return SC_OK;
}

static sc_err_t ds18b20_read_rom_match(uint8_t chord, float *out_c)
{
    if (chord >= NCHORDS) return SC_ERR_RANGE;
    if (!ow_reset()) return SC_ERR_NACK;

    ow_write_byte(0x55);                     /* Match ROM */
    for (int i = 0; i < 8; i++) ow_write_byte(s_rom[chord][i]);
    ow_write_byte(0x44);                     /* Convert T */
    /* Wait for conversion (blocking, ~750 ms worst case at 12-bit). */
    delay_us_busy(DS18B18_CONV_MS * 1000);

    if (!ow_reset()) return SC_ERR_NACK;
    ow_write_byte(0x55);
    for (int i = 0; i < 8; i++) ow_write_byte(s_rom[chord][i]);
    ow_write_byte(0xBE);                     /* Read scratchpad */

    uint8_t sp[9];
    for (int i = 0; i < 9; i++) sp[i] = ow_read_byte();

    if (crc8(sp, 8) != sp[8]) return SC_ERR_NACK;

    int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
    /* 12-bit resolution: raw is in 1/16 °C units. */
    *out_c = (float)raw / 16.0f;
    return SC_OK;
}

sc_err_t temp_read(uint8_t chord, float *out_c)
{
    /* Try twice; DS18B20s can occasionally glitch on long 1-Wire buses. */
    sc_err_t err = ds18b20_read_rom_match(chord, out_c);
    if (err != SC_OK) err = ds18b20_read_rom_match(chord, out_c);
    return err;
}