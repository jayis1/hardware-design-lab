/*
 * drivers/hx711.c — HX711 24-bit load cell ADC driver (bit-banged)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * HX711 is a 24-bit ADC designed for weigh scales. Interface is a simple
 * 2-wire serial: DOUT (data, PA4) and PD_SCK (power-down / clock, PA1).
 * Channel A gain 128 is used (the standard for load cells).
 *
 * Timing: 0.5 µs minimum clock high, 27 µs min between clocks.
 * 27+ consecutive high pulses put the chip in power-down mode (used here).
 */
#include "hx711.h"
#include "../registers.h"
#include "../board.h"

#define HX711_SCK_PIN  1  /* PA1 */
#define HX711_DOUT_PIN 4  /* PA4 */

static inline void sck_high(void) { GPIOA->BSRR = (1U << HX711_SCK_PIN); }
static inline void sck_low(void)  { GPIOA->BRR  = (1U << HX711_SCK_PIN); }
static inline int  dout_read(void) { return (GPIOA->IDR >> HX711_DOUT_PIN) & 1; }

/* Global calibration scale factor (set via app downlink tare). */
static int32_t g_offset = 0;
static float   g_scale  = 1.0f;   /* counts per gram — calibrated at factory */

void hx711_init(void)
{
    /* PA1 output (SCK), PA4 input (DOUT) */
    GPIOA->MODER &= ~(3U << (HX711_SCK_PIN*2));
    GPIOA->MODER |=  (GPIO_MODE_OUTPUT << (HX711_SCK_PIN*2));
    GPIOA->MODER &= ~(3U << (HX711_DOUT_PIN*2));  /* input */
    GPIOA->PUPDR |=  (1U << (HX711_DOUT_PIN*2));  /* pull-up */

    sck_low();
    hx711_power_up();
}

void hx711_power_down(void)
{
    sck_low();
    /* Need >27 pulses with SCK high at end to enter power-down */
    for (int i = 0; i < 30; i++) {
        sck_high();
        for (volatile int j = 0; j < 100; j++) { }  /* ~1 µs at 80 MHz */
        sck_low();
        for (volatile int j = 0; j < 100; j++) { }
    }
    /* Hold SCK high to keep in power-down */
    sck_high();
}

void hx711_power_up(void)
{
    sck_low();
    /* Datasheet: wait >400 µs after power-up before reading */
    for (volatile int i = 0; i < 4000; i++) { }
}

/* Wait for DOUT to go low (data ready), with timeout. */
int hx711_wait_ready(uint32_t timeout_ms)
{
    for (uint32_t i = 0; i < timeout_ms; i++) {
        if (!dout_read()) return 0;
        delay_ms(1);
    }
    return -1;
}

/* Read one 24-bit signed sample from channel A (gain 128, 25 pulses). */
int32_t hx711_read_raw(void)
{
    if (hx711_wait_ready(100) < 0) return HX711_READ_TIMEOUT;

    int32_t value = 0;
    for (int i = 0; i < 24; i++) {
        sck_high();
        for (volatile int j = 0; j < 40; j++) { }  /* ~0.5 µs */
        value = (value << 1) | dout_read();
        sck_low();
        for (volatile int j = 0; j < 40; j++) { }
    }

    /* 25th pulse sets channel A gain 128 for next read */
    sck_high();
    for (volatile int j = 0; j < 40; j++) { }
    sck_low();
    for (volatile int j = 0; j < 40; j++) { }

    /* Sign-extend 24-bit two's complement to 32-bit */
    if (value & 0x800000) value |= 0xFF000000;

    return value;
}

/*
 * Average N readings for noise reduction. Returns HX711_READ_TIMEOUT on failure.
 * The load cell signal is noisy at the gram level; averaging 4 reads gives
 * ±10 g resolution which is adequate for hive weight (tens of kg).
 */
int32_t hx711_read_avg(uint8_t n)
{
    int32_t sum = 0;
    int valid = 0;
    for (uint8_t i = 0; i < n; i++) {
        int32_t v = hx711_read_raw();
        if (v == HX711_READ_TIMEOUT) return HX711_READ_TIMEOUT;
        sum += v;
        valid++;
    }
    return sum / valid;
}

/* Convert raw reading to grams using calibration offset and scale. */
int32_t hx711_read_grams(void)
{
    int32_t raw = hx711_read_avg(4);
    if (raw == HX711_READ_TIMEOUT) return HX711_READ_TIMEOUT;
    return (int32_t)((raw - g_offset) / g_scale);
}

/* Tare: set the current raw reading as the zero point. */
void hx711_tare(void)
{
    int32_t raw = hx711_read_avg(8);
    if (raw != HX711_READ_TIMEOUT) g_offset = raw;
}

/* Set calibration scale (counts per gram). Called from factory calibration. */
void hx711_set_scale(float counts_per_gram)
{
    g_scale = counts_per_gram;
}

float hx711_get_scale(void) { return g_scale; }
int32_t hx711_get_offset(void) { return g_offset; }
void hx711_set_offset(int32_t off) { g_offset = off; }