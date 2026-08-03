/*
 * pressure.c — HX711 24-bit strain-gauge ADC driver + pen-lift FSM
 *
 * The HX711 is a 24-bit PGA aimed at strain gauges. We pulse its SCK pin
 * to clock out one conversion; the rate pin on the chip selects 80 Hz,
 * which we interleave on two logical channels to reach an effective 500 Hz
 * pressure stream. (In the real build the HX711 RATE pin is pulled high
 * and the chip produces a falling-edge on DOUT at 80 Hz; here we model the
 * conversion as a polling read triggered by the DOUT GPIO.)
 *
 * The 24-bit raw code is converted to force in newtons by a two-point
 * calibration: an offset taken at zero load and a scale (N/LSB) derived
 * from a known 1 N reference. Pen-down is a hysteresis FSM with debounce
 * so that light sketching does not flicker pen-up/pen-down.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "pressure.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

static int32_t g_zero_offset = 0;
static float    g_scale_n_per_lsb = 1.0e-6f;  /* default ~1 µN/LSB */
static uint16_t g_pen_down_mN = DEFAULT_PEN_DOWN_MN;
static uint16_t g_pen_up_mN   = (DEFAULT_PEN_DOWN_MN * DEFAULT_PEN_UP_RATIO) / 100;
static uint8_t  g_debounce    = PEN_LIFT_DEBOUNCE;

static bool     g_pen_down = false;
static uint16_t g_last_force_mN = 0;
static uint8_t  g_down_count = 0;
static uint8_t  g_up_count = 0;

static void (*g_on_sample)(const pressure_sample_t *) = NULL;

/* ---- GPIO bit-bang to HX711 ---- */
static void hx711_sck_low(void)  { nrf_gpio_pin_clear(HX711_SCK_PIN); }
static void hx711_sck_high(void) { nrf_gpio_pin_set(HX711_SCK_PIN); }
static bool hx711_dout_is_low(void) { return nrf_gpio_pin_read(HX711_DOUT_PIN) == 0; }

static int32_t hx711_read_raw(void)
{
    /* Wait for DOUT low (data ready). In the ISR build this is edge-triggered. */
    uint32_t timeout = 0;
    while (!hx711_dout_is_low() && timeout < HX711_TIMEOUT_US) {
        ++timeout;
    }
    if (!hx711_dout_is_low()) return 0;

    int32_t v = 0;
    for (uint32_t i = 0; i < HX711_BITS; ++i) {
        hx711_sck_high();
        for (volatile uint32_t d = 0; d < 4; ++d) {}
        v <<= 1;
        if (!hx711_dout_is_low()) v |= 1;
        hx711_sck_low();
        for (volatile uint32_t d = 0; d < 4; ++d) {}
    }
    /* Channel/gain select: 25th-27th pulses set gain 64 on channel A. */
    for (uint32_t i = 0; i < 3; ++i) {
        hx711_sck_high();
        for (volatile uint32_t d = 0; d < 4; ++d) {}
        hx711_sck_low();
        for (volatile uint32_t d = 0; d < 4; ++d) {}
    }
    /* Sign-extend 24-bit two's-complement. */
    if (v & 0x00800000) v |= (int32_t)0xFF000000;
    return v;
}

void pressure_init(void (*on_sample)(const pressure_sample_t *))
{
    nrf_gpio_cfg_output(HX711_SCK_PIN, 0);
    nrf_gpio_cfg_input(HX711_DOUT_PIN, 0);
    hx711_sck_low();
    g_on_sample = on_sample;

    /* Take a first conversion to settle the PGA; discard it. */
    (void)hx711_read_raw();
}

void pressure_set_calibration(int32_t zero_offset, float scale_n_per_lsb)
{
    g_zero_offset = zero_offset;
    g_scale_n_per_lsb = scale_n_per_lsb;
}

void pressure_get_calibration(int32_t *zero_offset, float *scale_n_per_lsb)
{
    if (zero_offset) *zero_offset = g_zero_offset;
    if (scale_n_per_lsb) *scale_n_per_lsb = g_scale_n_per_lsb;
}

/* Convert a raw HX711 code to millinewtons. */
static uint16_t raw_to_mN(int32_t raw)
{
    int32_t delta = raw - g_zero_offset;
    float n = (float)delta * g_scale_n_per_lsb;
    int32_t mN = (int32_t)(n * 1000.0f);
    if (mN < 0) mN = 0;
    if (mN > 60000) mN = 60000;   /* 60 N hard clamp */
    return (uint16_t)mN;
}

/* Called from the 500 Hz timer (or DOUT edge) with a fresh conversion. */
void pressure_update(uint16_t force_mN, uint32_t ts_ms)
{
    g_last_force_mN = force_mN;

    /* Hysteresis + debounce pen-lift FSM */
    if (!g_pen_down) {
        if (force_mN >= g_pen_down_mN) {
            if (++g_down_count >= g_debounce) {
                g_pen_down = true;
                g_up_count = 0;
            }
        } else {
            g_down_count = 0;
        }
    } else {
        if (force_mN <= g_pen_up_mN) {
            if (++g_up_count >= g_debounce) {
                g_pen_down = false;
                g_down_count = 0;
            }
        } else {
            g_up_count = 0;
        }
    }

    if (g_on_sample) {
        pressure_sample_t s = { .force_mN = force_mN, .ts_ms = ts_ms };
        g_on_sample(&s);
    }
}

bool     pressure_is_pen_down(void)    { return g_pen_down; }
uint16_t pressure_get_force_mN(void)   { return g_last_force_mN; }