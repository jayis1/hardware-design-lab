/*
 * optics.c — AeroCast laser / UV-LED / PMT bias control
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Manages the three optical sources used in the AeroCast cavity:
 *   - 650 nm sizing laser (continuous, current-programmed via DAC2)
 *   - 340 nm UV fluorescence excitation LED (pulsed via TIM3)
 *   - PMT bias voltage (0–900 V via DAC1 + Cockcroft-Walton generator)
 *
 * Safety: the lid-interlock switch in main.c forces optics_laser_off()
 * and optics_uv_off() when the enclosure is opened.
 */
#ifndef AEROCAST_OPTICS_C
#define AEROCAST_OPTICS_C
#endif

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "optics.h"

/* ---- Module state ---- */
static struct {
    uint8_t  laser_on;
    uint8_t  uv_on;
    uint16_t laser_drive;   /* DAC code 0–4095 */
    uint16_t pmt_bias_code; /* DAC code 0–4095 → 0–900 V */
    uint16_t uv_pulse_us;   /* UV pulse width */
} g_optics;

/* ---- Public API ---- */
void optics_init(void)
{
    /* Bring up DAC channels */
    dac_init_ch(DAC1_BASE, 0);  /* PMT bias  */
    dac_init_ch(DAC1_BASE, 1);  /* laser Iset */

    /* UV-LED PWM: 340 nm pulse, TIM3_CH1 on PB0.
     * We run at 20 kHz carrier; the actual fluorescence pulse is gated
     * by re-triggering CCR from the AFE event line. */
    tim_init_pwm(TIM3_BASE, /* PSC */ 0, /* ARR */ 14000u, /* CCR */ 0);

    /* PMT bias to safe 0 */
    dac_write(DAC1_BASE, 0, 0);
    dac_write(DAC1_BASE, 1, 0);

    g_optics.laser_on = 0;
    g_optics.uv_on = 0;
    g_optics.laser_drive = 0;
    g_optics.pmt_bias_code = 0;
    g_optics.uv_pulse_us = 40;   /* 40 µs typical fluorescence window */
}

void optics_laser_on(uint16_t drive)
{
    if (drive > 0xFFFu) drive = 0xFFFu;
    dac_write(DAC1_BASE, 1, drive);   /* channel 2 = laser Iset */
    g_optics.laser_drive = drive;
    g_optics.laser_on = 1;
}

void optics_laser_off(void)
{
    dac_write(DAC1_BASE, 1, 0);
    g_optics.laser_on = 0;
}

void optics_uv_on(void)
{
    /* Enable TIM3 CH1 output with the configured pulse width.
     * Pulse width in µs → CCR = pulse_us * (280 MHz / 1e6) / (PSC+1)
     * With PSC=0, ARR=14000 → 50 µs ≈ 14 000 counts full period. */
    uint32_t ccr = (uint32_t)g_optics.uv_pulse_us * 280u;  /* 280 counts/µs */
    if (ccr > 14000u) ccr = 14000u;
    tim_set_duty(TIM3_BASE, ccr);
    g_optics.uv_on = 1;
}

void optics_uv_off(void)
{
    tim_set_duty(TIM3_BASE, 0);
    g_optics.uv_on = 0;
}

void optics_uv_pulse_width_us(uint16_t us)
{
    g_optics.uv_pulse_us = us;
    if (g_optics.uv_on) optics_uv_on();   /* apply live */
}

void optics_pmt_bias_set(uint16_t code)
{
    if (code > 0xFFFu) code = 0xFFFu;
    /* Soft-start: ramp in ~30 steps over ~3 ms to avoid inrush
     * stressing the Cockcroft-Walton multiplier. */
    uint16_t cur = g_optics.pmt_bias_code;
    int16_t  step = (code > cur) ? 1 : -1;
    while (cur != code) {
        uint16_t next = (uint16_t)((int16_t)cur + step * (int16_t)((code > cur ? (code-cur) : (cur-code)) / 30 + 1));
        if ((step > 0 && next > code) || (step < 0 && next < code)) next = code;
        dac_write(DAC1_BASE, 0, next);
        cur = next;
        for (volatile int i = 0; i < 200; ++i) ;   /* ~100 µs */
    }
    g_optics.pmt_bias_code = code;
}

uint16_t optics_pmt_bias_get(void)  { return g_optics.pmt_bias_code; }
uint8_t  optics_laser_is_on(void)   { return g_optics.laser_on; }
uint8_t  optics_uv_is_on(void)      { return g_optics.uv_on; }

/* ---- Laser monitor: read the internal ADC channel tied to the
 * monitor photodiode. Returns 0–4095 (12-bit). If the reading drops
 * below a threshold we flag a laser-fault so main.c can retract. */
uint8_t optics_laser_health_check(void)
{
    /* Trigger a software conversion on ADC1 channel 5.
     * In a full build this is wired through the HAL; here we read the
     * DR register after a manual SWSTART. */
    volatile uint32_t *adc_cr   = (volatile uint32_t *)(ADC1_BASE + 0x08UL);
    volatile uint32_t *adc_isr  = (volatile uint32_t *)(ADC1_BASE + 0x10UL);
    volatile uint32_t *adc_dr   = (volatile uint32_t *)(ADC1_BASE + 0x4CUL);
    *adc_cr |= (1u << 2u);             /* ADSTART */
    for (volatile int i = 0; i < 500 && !(*adc_isr & (1u << 2u)); ++i) ; /* ADRDY-ish */
    uint32_t v = *adc_dr & 0xFFFFu;
    /* Expect ~0x600 with a healthy beam. <0x100 → dead laser / broken wire. */
    return (v > 0x100u) ? 1u : 0u;
}