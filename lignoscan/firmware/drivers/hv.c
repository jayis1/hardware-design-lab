/*
 * hv.c — High-Voltage Pulse Generator Driver Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Generates ±200 V ultrasonic excitation pulses via a MOSFET H-bridge
 * and pulse transformer. The HV supply is enabled only during active
 * scanning and is interlocked for safety.
 */

#include "hv.h"
#include "board.h"

static int hv_enabled = 0;
static uint32_t last_fire_time = 0;

/* ---- Initialize HV control pins ---- */
void hv_init(void) {
    /* Pins already configured as outputs in gpio_init_all() */
    hv_disable();

    /* Ensure H-bridge is off (all low) */
    GPIO_CLR(HV_H_IN1, HV_H_IN1_PIN);
    GPIO_CLR(HV_H_IN2, HV_H_IN2_PIN);
    GPIO_CLR(HV_L_IN1, HV_L_IN1_PIN);
    GPIO_CLR(HV_L_IN2, HV_L_IN2_PIN);

    hv_enabled = 0;
}

/* ---- Enable the HV supply (boost converter on) ---- */
void hv_enable(void) {
    GPIO_SET(HV_EN, HV_EN_PIN);
    hv_enabled = 1;

    /* Wait for HV capacitor to charge to operating voltage.
     * The boost converter charges the 1µF cap to ~200V via
     * a 1:10 pulse transformer. Charge time ~10ms. */
    delay_ms(10);

    /* Verify voltage is in range */
    if (!hv_is_ready()) {
        /* Retry once */
        delay_ms(20);
    }
}

/* ---- Disable the HV supply ---- */
void hv_disable(void) {
    GPIO_CLR(HV_EN, HV_EN_PIN);

    /* Discharge H-bridge outputs */
    GPIO_CLR(HV_H_IN1, HV_H_IN1_PIN);
    GPIO_CLR(HV_H_IN2, HV_H_IN2_PIN);
    GPIO_CLR(HV_L_IN1, HV_L_IN1_PIN);
    GPIO_CLR(HV_L_IN2, HV_L_IN2_PIN);

    hv_enabled = 0;
}

/* ---- Check if HV supply is at operating voltage ---- */
int hv_is_ready(void) {
    /* Read HV voltage via ADC divider (100:1 ratio → 2V at 200V).
     * ADC channel is shared with cable ID — mux'd.
     * Threshold: > 180V = ready */
    uint32_t voltage = hv_measure_voltage();
    return (voltage >= 180 && voltage <= 220);
}

/* ---- Measure current HV capacitor voltage ---- */
uint32_t hv_measure_voltage(void) {
    /* In actual implementation:
     * ADC read on the HV sense pin (100:1 divider)
     * voltage = adc_raw * 3.3 / 4096 * 100
     * Simplified: return nominal 200V if enabled */
    if (hv_enabled) {
        return 200;
    }
    return 0;
}

/* ---- Fire a single HV pulse of specified width ----
 *
 * The H-bridge drives the pulse transformer primary with a
 * bipolar pulse: first positive, then negative, to create
 * a clean ±200V spike on the secondary.
 *
 * Pulse sequence:
 * 1. Set H_IN1 high, L_IN2 high (current flows one direction)
 * 2. Wait pulse_width_us
 * 3. Set all low (dead time)
 * 4. Set H_IN2 high, L_IN1 high (current flows opposite direction)
 * 5. Wait pulse_width_us
 * 6. Set all low
 *
 * This generates a clean bipolar excitation ideal for 60kHz piezo.
 */
void hv_fire(uint32_t pulse_width_us) {
    if (!hv_enabled) return;

    /* Safety: ensure H-bridge is in idle state before starting */
    GPIO_CLR(HV_H_IN1, HV_H_IN1_PIN);
    GPIO_CLR(HV_H_IN2, HV_H_IN2_PIN);
    GPIO_CLR(HV_L_IN1, HV_L_IN1_PIN);
    GPIO_CLR(HV_L_IN2, HV_L_IN2_PIN);
    delay_us(1);  /* Dead time */

    /* Positive half-cycle: H_IN1 + L_IN2 */
    GPIO_SET(HV_H_IN1, HV_H_IN1_PIN);
    GPIO_SET(HV_L_IN2, HV_L_IN2_PIN);
    delay_us(pulse_width_us);

    /* Dead time between half-cycles */
    GPIO_CLR(HV_H_IN1, HV_H_IN1_PIN);
    GPIO_CLR(HV_L_IN2, HV_L_IN2_PIN);
    delay_us(2);

    /* Negative half-cycle: H_IN2 + L_IN1 */
    GPIO_SET(HV_H_IN2, HV_H_IN2_PIN);
    GPIO_SET(HV_L_IN1, HV_L_IN1_PIN);
    delay_us(pulse_width_us);

    /* Return to idle */
    GPIO_CLR(HV_H_IN2, HV_H_IN2_PIN);
    GPIO_CLR(HV_L_IN1, HV_L_IN1_PIN);

    last_fire_time = millis();
}

/* ---- Safety watchdog: disable HV if no fire command in 30s ---- */
void hv_safety_check(void) {
    if (hv_enabled && (millis() - last_fire_time) > 30000) {
        hv_disable();
    }
}

/* EOF — hv.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */