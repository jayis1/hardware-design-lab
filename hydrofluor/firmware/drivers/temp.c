/*
 * temp.c — HydroFluor DS18B20 1-Wire bench temperature driver
 * Author: jayis1
 * License: MIT
 *
 * Bit-banged 1-Wire protocol on PB8 to read the DS18B20 temperature sensor
 * mounted on the optical bench. Used for per-cycle temperature compensation
 * of fluorescence quantum yield. The DS18B20 provides 12-bit resolution
 * (0.0625 °C). Returns temperature in 0.01 °C units (×100).
 */
#include "temp.h"
#include "../registers.h"

static void ow_drive_low(void)
{
    gpio_mode(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_MODE_OUTPUT);
    gpio_clr(ONEWIRE_PORT, ONEWIRE_PIN);
}
static void ow_release(void)
{
    gpio_mode(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_MODE_INPUT);
    gpio_pupd(ONEWIRE_PORT, ONEWIRE_PIN, GPIO_PUPD_PULLUP);
}
static uint8_t ow_read_bit(void)
{
    ow_drive_low();
    for (volatile int i = 0; i < 2; ++i) { }   /* ~1 µs */
    ow_release();
    for (volatile int i = 0; i < 20; ++i) { } /* ~15 µs sample window */
    return gpio_read(ONEWIRE_PORT, ONEWIRE_PIN);
}

static int ow_reset(void)
{
    ow_drive_low();
    for (volatile int i = 0; i < 600; ++i) { }  /* ~500 µs */
    ow_release();
    for (volatile int i = 0; i < 80; ++i) { }   /* ~65 µs */
    uint8_t presence = gpio_read(ONEWIRE_PORT, ONEWIRE_PIN);
    for (volatile int i = 0; i < 500; ++i) { }  /* wait rest */
    return presence ? -1 : 0;   /* low = presence pulse OK */
}

static void ow_write_byte(uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        ow_drive_low();
        for (volatile int j = 0; j < 2; ++j) { }
        if (b & 1) ow_release();   /* write 1: release quickly */
        for (volatile int j = 0; j < 80; ++j) { } /* ~70 µs slot */
        if (!(b & 1)) ow_release();
        for (volatile int j = 0; j < 2; ++j) { } /* recovery */
        b >>= 1;
    }
}

static uint8_t ow_read_byte(void)
{
    uint8_t b = 0;
    for (int i = 0; i < 8; ++i) {
        b >>= 1;
        if (ow_read_bit()) b |= 0x80;
        for (volatile int j = 0; j < 60; ++j) { } /* slot */
    }
    return b;
}

void temp_init(void)
{
    RCC->AHB2ENR |= (1U << 1);  /* GPIOB */
    ow_release();
}

int16_t temp_read_c01(void)
{
    if (ow_reset() < 0) return -10000;   /* no device */

    ow_write_byte(0xCC);   /* Skip ROM (only one device on bus) */
    ow_write_byte(DS18B20_CMD_CONVERT);
    /* Wait for conversion (12-bit → ~750 ms). We block for simplicity;
     * in deployment firmware this is deferred to the next cycle. */
    for (volatile int i = 0; i < 600000; ++i) { }

    if (ow_reset() < 0) return -10000;
    ow_write_byte(0xCC);
    ow_write_byte(DS18B20_CMD_READ_SP);
    uint8_t lsb = ow_read_byte();
    uint8_t msb = ow_read_byte();
    int16_t raw = (int16_t)((msb << 8) | lsb);
    /* DS18B20 12-bit resolution: raw × 0.0625 °C. Convert to 0.01 °C:
     * temp_c01 = raw × 6.25 ≈ raw × 6 + raw/4 */
    int16_t t01 = (int16_t)((raw * 625) / 100);  /* raw × 6.25 */
    return t01;
}