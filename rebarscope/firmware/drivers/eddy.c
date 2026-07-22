/*
 * drivers/eddy.c — Pulsed eddy-current rebar cover-depth / diameter estimator
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * The excitation coil is driven by an AD9833 DDS + OPA569 power op-amp at
 * 1 kHz with a 100 µs current ramp (0 → 100 mA). The induced voltage on a
 * sense coil is demodulated synchronously and sampled at 100 kHz by the
 * ADS1220 (in high-speed mode) for 5 ms; the peak time t_p of the decay
 * envelope is proportional to the cover depth. A piecewise-linear
 * calibration curve converts t_p to depth in millimetres.
 */
#include "../board.h"
#include "../registers.h"
#include "eddy.h"
#include "ads1220.h"
#include <math.h>

/* AD9833 control word */
#define AD9833_CMD_RESET    0x2100
#define AD9833_CMD_FREQ28   0x2000
#define AD9833_CMD_PHASE    0xC000
#define AD9833_CTRL_SINE   0x2000

/* Eddy driver peak current (mA) and pulse duration (µs) */
#define EDDY_PEAK_MA       100.0f
#define EDDY_PULSE_US      100u

/* Calibration table — t_p (µs) → cover depth (mm).
 * Populated by eddy_calibrate() against a reference block. */
#define EDDY_CAL_N 4
typedef struct {
    float t_p_us;
    float depth_mm;
} eddy_cal_point_t;

static eddy_cal_point_t cal_table[EDDY_CAL_N] = {
    { 12.0f, 12.0f },
    { 28.0f, 25.0f },
    { 48.0f, 38.0f },
    { 72.0f, 50.0f },
};

/* Per-bar diameter calibration: amplitude (a.u.) → diameter (mm) */
static const float diam_table[3][2] = {
    { 0.30f, 13.0f },   /* #4 */
    { 0.55f, 19.0f },   /* #6 */
    { 0.85f, 25.0f },   /* #8 */
};

static void spi3_cs_low(void)
{
    GPIOE->BSRR = (1u << (4 + 16));
}
static void spi3_cs_high(void)
{
    GPIOE->BSRR = (1u << 4);
}
static uint8_t spi3_xfer(uint8_t tx)
{
    /* SPI3 — reuse same pattern; assume SPI3 already enabled */
    while (!(SPI3->SR & SPI_SR_TXE)) ;
    *(volatile uint8_t *)&SPI3->DR = tx;
    while (!(SPI3->SR & SPI_SR_RXNE)) ;
    return *(volatile uint8_t *)&SPI3->DR;
}

static void eddy_delay_us(uint32_t us)
{
    volatile uint32_t n = us * 20u;
    while (n--) __asm volatile("nop");
}

void eddy_init(void)
{
    pwr_gate_enable(PIN_PWR_EDDY, 1);
    eddy_delay_us(500);

    /* Reset AD9833 */
    spi3_cs_low();
    spi3_xfer(0x20); spi3_xfer(0x00);
    spi3_cs_high();
    spi3_cs_low();
    spi3_xfer(AD9833_CMD_RESET >> 8); spi3_xfer(AD9833_CMD_RESET & 0xFF);
    spi3_cs_high();

    /* Set frequency: 1000 Hz on 25 MHz MCLK → freqreg = (f * 2^28)/MCLK
     * f=1000 → 0x0A3D7
     */
    uint32_t freq = (uint32_t)(1000.0f * 268435456.0f / 25000000.0f);
    spi3_cs_low();
    spi3_xfer(0x20); spi3_xfer(0x00);
    spi3_xfer((uint8_t)(freq & 0xFF));
    spi3_xfer((uint8_t)((freq >> 8) & 0x3F) | 0x40);
    spi3_xfer((uint8_t)((freq >> 14) & 0xFF));
    spi3_xfer((uint8_t)((freq >> 22) & 0x3F) | 0x10);
    spi3_cs_high();

    /* Enable sine output */
    spi3_cs_low();
    spi3_xfer(AD9833_CTRL_SINE >> 8); spi3_xfer(AD9833_CTRL_SINE & 0xFF);
    spi3_cs_high();
}

/* Drive a single current pulse into the coil */
static void eddy_fire_pulse(void)
{
    /* Enable OPA569 driver */
    GPIOC->BSRR = (1u << 3);
    eddy_delay_us(EDDY_PULSE_US);
    /* Disable driver — coil decays */
    GPIOC->BRR = (1u << 3);
}

/* Capture the decay waveform on ADS1220 (switched to eddy demod channel) */
static void eddy_capture(uint16_t *buf, uint16_t n)
{
    ads1220_select_mux(0x4u);   /* AIN2/AIN3 = eddy demod */
    for (uint16_t i = 0; i < n; i++) {
        int32_t raw = ads1220_read_raw();
        /* normalize to 16-bit signed */
        int16_t s = (int16_t)(raw >> 8);
        buf[i] = (uint16_t)s;
        eddy_delay_us(10);   /* ~100 kHz sample rate */
    }
    ads1220_select_mux(0x0u);   /* back to HCP */
}

static float eddy_find_peak_time_us(const uint16_t *buf, uint16_t n)
{
    /* Simple envelope follower: take abs value, track running max */
    int16_t max_val = 0;
    uint16_t max_idx = 0;
    for (uint16_t i = 0; i < n; i++) {
        int16_t s = (int16_t)buf[i];
        int16_t a = (s < 0) ? (int16_t)(-s) : s;
        if (a > max_val) {
            max_val = a;
            max_idx = i;
        }
    }
    /* sample interval 10 µs → peak time in µs */
    return (float)max_idx * 10.0f;
}

float eddy_measure_depth_mm(float *out_amp)
{
    uint16_t buf[500];
    eddy_fire_pulse();
    eddy_capture(buf, 500);

    float t_p = eddy_find_peak_time_us(buf, 500);
    /* amplitude (arbitrary units, 0..1) */
    int16_t max_val = 0;
    for (uint16_t i = 0; i < 500; i++) {
        int16_t s = (int16_t)buf[i];
        int16_t a = (s < 0) ? (int16_t)(-s) : s;
        if (a > max_val) max_val = a;
    }
    float amp = (float)max_val / 32767.0f;
    if (out_amp) *out_amp = amp;

    /* Piecewise-linear interpolation on cal_table */
    if (t_p <= cal_table[0].t_p_us)
        return cal_table[0].depth_mm;
    if (t_p >= cal_table[EDDY_CAL_N - 1].t_p_us)
        return cal_table[EDDY_CAL_N - 1].depth_mm;
    for (int i = 0; i < EDDY_CAL_N - 1; i++) {
        if (t_p >= cal_table[i].t_p_us && t_p <= cal_table[i+1].t_p_us) {
            float frac = (t_p - cal_table[i].t_p_us) /
                         (cal_table[i+1].t_p_us - cal_table[i].t_p_us);
            return cal_table[i].depth_mm +
                   frac * (cal_table[i+1].depth_mm - cal_table[i].depth_mm);
        }
    }
    return -1.0f;
}

float eddy_estimate_diameter_mm(float amp, float depth_mm)
{
    /* Amplitude falls with depth^3; normalize */
    if (depth_mm < 1.0f) return 0.0f;
    float norm = amp * (depth_mm * depth_mm * depth_mm) / 1000.0f;
    float best_d = 0.0f;
    float best_err = 1e9f;
    for (int i = 0; i < 3; i++) {
        float err = fabsf(norm - diam_table[i][0]);
        if (err < best_err) {
            best_err = err;
            best_d = diam_table[i][1];
        }
    }
    return best_d;
}

void eddy_update_calibration(const float *t_p_us, const float *depth_mm, uint8_t n)
{
    if (n > EDDY_CAL_N) n = EDDY_CAL_N;
    for (uint8_t i = 0; i < n; i++) {
        cal_table[i].t_p_us   = t_p_us[i];
        cal_table[i].depth_mm = depth_mm[i];
    }
}

void eddy_powerdown(void)
{
    /* disable OPA569 + AD9833 */
    GPIOC->BRR = (1u << 3);
    pwr_gate_enable(PIN_PWR_EDDY, 0);
}