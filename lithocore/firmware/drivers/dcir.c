/*
 * dcir.c — DC Internal Resistance measurement.
 *
 * Applies a 2 A, 100 ms discharge pulse via the DCIR FET gate (PA8,
 * TIM1_CH1) and measures the voltage response to compute the cell's
 * DC internal resistance.
 *
 * The measurement sequence:
 *   1. Measure V_before (cell at rest).
 *   2. Trigger the 2 A discharge pulse (TIM1 one-shot, 100 ms).
 *   3. At t = 90 ms (mid-pulse, after inductive transient settles):
 *      measure V_during.
 *   4. At t = 200 ms (100 ms after pulse ends): measure V_after.
 *   5. DCIR = (V_before - V_during) / I_pulse
 *   6. The V_after measurement allows separation of ohmic vs.
 *      polarization resistance (V_before - V_after = polarization drop).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "dcir.h"
#include "../board.h"
#include "../registers.h"
#include "safety.h"

/* -------------------------------------------------------------------------
 * DCIR pulse via TIM1 Channel 1
 *
 * TIM1 is configured as a one-shot: the CCR1 duty cycle sets the pulse
 * duration, and the MOE (main output enable) bit gates the output.
 * The FET connects the cell to the 0.5 Ω power resistor (from the
 * supercap), giving ~2 A at 4.0 V cell voltage (4.0 / (0.5 + DCIR)).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void dcir_pulse_init(void)
{
    /* TIM1: up-counting, PSC = 0 (163 MHz), ARR = 16300000 (100 ms period).
       Actually, we want a one-shot 100 ms pulse.
       PSC = 1700-1 → 163.84 MHz / 1700 = 96376 Hz tick → ARR = 9638 for 100 ms.
       Let's use PSC = 16383 → 10 kHz tick → ARR = 1000 for 100 ms. */
    TIM1->PSC = 16383;        /* 10 kHz tick (0.1 ms per tick) */
    TIM1->ARR = 1200;         /* 120 ms total period (pulse + relax) */
    TIM1->CCR1 = 1000;        /* Pulse high for 100 ms (1000 ticks), then off */

    /* PWM mode 1 on channel 1 */
    TIM1->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1->CCER = TIM_CCER_CC1E;   /* enable CH1 output */
    TIM1->BDTR = TIM_BDTR_MOE;    /* main output enable (advanced timer) */
    TIM1->CR1 = TIM_CR1_ARPE;     /* auto-reload preload */
}

static void dcir_pulse_trigger(void)
{
    /* Reset counter and start */
    TIM1->CNT = 0;
    TIM1->EGR = 1;           /* force update event */
    TIM1->CR1 |= TIM_CR1_CEN; /* start timer */
}

static void dcir_pulse_wait(void)
{
    /* Wait for the timer period to complete ( UIF flag ) */
    while (!(TIM1->SR & TIM_SR_UIF)) { }
    TIM1->SR = 0;            /* clear flags */
    TIM1->CR1 &= ~TIM_CR1_CEN; /* stop timer */
}

/* -------------------------------------------------------------------------
 * DCIR measurement
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int dcir_measure(uint16_t *dcir_mohm)
{
    uint16_t v_before, v_during, v_after;
    int32_t  dv_ohmic, dv_polar;
    int32_t  dcir_uohm;

    /* Measure V_before (cell at rest) */
    if (safety_read_voltage(&v_before) != SAFETY_OK)
        return DCIR_ERROR_ADC;

    /* Initialize and trigger the pulse */
    dcir_pulse_init();
    dcir_pulse_trigger();

    /* Wait 90 ms into the pulse, then measure V_during */
    delay_ms(90);
    if (safety_read_voltage(&v_during) != SAFETY_OK) {
        dcir_pulse_trigger();  /* clear the timer */
        return DCIR_ERROR_ADC;
    }

    /* Wait for the pulse to complete (total 120 ms) */
    dcir_pulse_wait();

    /* Wait 100 ms for relaxation, then measure V_after */
    delay_ms(100);
    if (safety_read_voltage(&v_after) != SAFETY_OK)
        return DCIR_ERROR_ADC;

    /* Compute DCIR:
     *   DCIR_ohmic = (V_before - V_during) / I_pulse
     *   DCIR_polar = (V_before - V_after) / I_pulse
     *   DCIR_total = (V_before - V_after_steady) / I_pulse
     *
     * I_pulse = 2 A (constant, set by the 0.5 Ω resistor + supercap voltage).
     * V in mV, I in A → DCIR in mΩ: DCIR_mohm = (V_before - V_during) / 2
     *
     * We report the ohmic DCIR (before - during) as it's the standard. */
    dv_ohmic = (int32_t)v_before - (int32_t)v_during;
    dv_polar = (int32_t)v_before - (int32_t)v_after;

    /* DCIR in mΩ = ΔV(mV) / I(A) = ΔV / 2 */
    dcir_uohm = dv_ohmic * 500;   /* µΩ = mV × 1000 / 2A = mV × 500 */

    /* Sanity check: typical 18650 DCIR = 20–80 mΩ = 20000–80000 µΩ.
     * V_drop = 2A × 50mΩ = 100 mV. If dv < 10 mV or > 500 mV, suspect error. */
    if (dv_ohmic < 10 || dv_ohmic > 500) {
        *dcir_mohm = 0;
        return DCIR_ERROR_PULSE;
    }

    /* Convert µΩ to mΩ (store as mΩ for consistency with the Randles params) */
    *dcir_mohm = (uint16_t)(dcir_uohm / 1000);

    (void)dv_polar;  /* available for advanced analysis if needed */

    return DCIR_OK;
}