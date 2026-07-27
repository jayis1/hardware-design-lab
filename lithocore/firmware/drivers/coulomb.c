/*
 * coulomb.c — Coulomb counter for capacity estimation.
 *
 * Performs a partial discharge of the cell while integrating the current
 * over time, then extrapolates the full capacity from the OCV drop and
 * the Coulomb count.
 *
 * This is an optional measurement — the core EIS sweep does not require it.
 * It's useful for cells where the SoH score is borderline and the user
 * wants a direct capacity measurement.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "coulomb.h"
#include "../board.h"
#include "safety.h"
#include "dcir.h"

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */
static volatile uint8_t  g_coulomb_active = 0;
static volatile uint8_t  g_coulomb_abort = 0;
static volatile uint32_t g_coulomb_start_tick = 0;
static volatile uint32_t g_coulomb_duration_ms = 0;
static volatile uint16_t g_coulomb_current_ma = 0;
static volatile int64_t  g_coulomb_charge_uah = 0;  /* accumulated µAh */
static volatile uint16_t g_coulomb_start_mv = 0;
static volatile uint16_t g_coulomb_end_mv = 0;

/* -------------------------------------------------------------------------
 * Coulomb counter init
 * ------------------------------------------------------------------------- */
int coulomb_init(void)
{
    g_coulomb_active = 0;
    g_coulomb_abort = 0;
    g_coulomb_charge_uah = 0;
    return 0;
}

/* -------------------------------------------------------------------------
 * Start a partial discharge
 *
 * Discharges the cell at a constant current for a specified duration,
 * integrating the total extracted charge. The discharge uses the same
 * DCIR pulse FET but at a controlled duty cycle (PWM) to regulate current.
 *
 * Current regulation: the firmware monitors the actual current via the
 * I-sense ADC and adjusts the PWM duty cycle in a PI control loop to
 * maintain the target current.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int coulomb_start_discharge(uint16_t current_ma, uint32_t duration_ms)
{
    if (g_coulomb_active)
        return -1;

    /* Safety check before starting */
    uint16_t v, t;
    if (safety_check(&v, &t) != SAFETY_OK)
        return -1;

    g_coulomb_active = 1;
    g_coulomb_abort = 0;
    g_coulomb_start_tick = g_ticks;  /* from main.c */
    g_coulomb_duration_ms = duration_ms;
    g_coulomb_current_ma = current_ma;
    g_coulomb_start_mv = v;
    g_coulomb_charge_uah = 0;

    /* Configure TIM1 for PWM discharge at 1 kHz, 50% duty initially */
    TIM1->PSC = 16383;          /* 10 kHz tick */
    TIM1->ARR = 10;             /* 1 kHz PWM */
    TIM1->CCR1 = 5;             /* 50% duty start */
    TIM1->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1->CCER = TIM_CCER_CC1E;
    TIM1->BDTR = TIM_BDTR_MOE;
    TIM1->CR1 = TIM_CR1_ARPE | TIM_CR1_CEN;

    /* Discharge loop */
    while (g_coulomb_active && !g_coulomb_abort) {
        uint32_t elapsed = g_ticks - g_coulomb_start_tick;
        if (elapsed >= g_coulomb_duration_ms)
            break;

        /* Safety check every second */
        if (safety_check(&v, &t) != SAFETY_OK) {
            g_coulomb_abort = 1;
            break;
        }

        /* Stop if voltage drops below 2.5 V (deep discharge protection) */
        if (v < 2500) {
            g_coulomb_abort = 1;
            break;
        }

        /* Read actual current (simplified — read I-sense ADC channel) */
        ADC1->SQR1 = (2U << 6);  /* channel 2 = I_sense */
        ADC1->CR |= ADC_CR_ADSTART;
        while (!(ADC1->ISR & ADC_ISR_EOC)) { }
        uint16_t i_adc = (uint16_t)ADC1->DR;

        /* Convert ADC to mA (calibration-dependent; assume 1 mA/LSB for now) */
        int32_t actual_ma = (int32_t)i_adc - 2048;  /* centered at 2048 */
        if (actual_ma < 0) actual_ma = 0;

        /* PI controller: adjust PWM duty to match target current */
        int32_t error = (int32_t)g_coulomb_current_ma - actual_ma;
        static int32_t integral = 0;
        integral += error;
        if (integral > 1000) integral = 1000;
        if (integral < -1000) integral = -1000;

        int32_t duty = 5 + error / 100 + integral / 200;
        if (duty < 1) duty = 1;
        if (duty > 9) duty = 9;
        TIM1->CCR1 = (uint32_t)duty;

        /* Integrate charge: Q += I × dt
         * dt = 10 ms (loop period), I in mA → Q in mA·ms = µC
         * Convert to µAh: µAh = µC / 3600 */
        g_coulomb_charge_uah += (int64_t)actual_ma * 10 / 36;

        /* Wait 10 ms */
        delay_ms(10);
    }

    /* Stop discharge */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->CCR1 = 0;

    /* Measure end voltage */
    safety_read_voltage(&g_coulomb_end_mv);
    g_coulomb_active = 0;

    return 0;
}

/* -------------------------------------------------------------------------
 * Get result with extrapolated capacity
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int coulomb_get_result(coulomb_result_t *result)
{
    result->total_mah_discharged = (uint32_t)(g_coulomb_charge_uah / 1000);
    result->duration_ms = g_ticks - g_coulomb_start_tick;
    result->start_mv = g_coulomb_start_mv;
    result->end_mv = g_coulomb_end_mv;

    /* Extrapolate full capacity:
     * We discharged from start_mv to end_mv, extracting total_mah.
     * The full capacity is estimated by mapping the voltage drop to
     * the SoC range using the OCV curve.
     *
     * For a simplified linear approximation:
     *   SoC_start = ocv_to_soc(start_mv)
     *   SoC_end = ocv_to_soc(end_mv)
     *   capacity = total_mah / (SoC_start - SoC_end)
     *
     * The OCV-to-SoC mapping is chemistry-dependent. Here we use a
     * simplified linear model for NMC (3.0–4.2 V = 0–100 %). */
    int32_t soc_start = ((int32_t)g_coulomb_start_mv - 3000) * 100 / 1200;
    int32_t soc_end   = ((int32_t)g_coulomb_end_mv - 3000) * 100 / 1200;
    if (soc_start > 100) soc_start = 100;
    if (soc_start < 0)   soc_start = 0;
    if (soc_end > 100)   soc_end = 100;
    if (soc_end < 0)     soc_end = 0;

    int32_t soc_drop = soc_start - soc_end;
    if (soc_drop > 0) {
        result->estimated_capacity_mah =
            (uint16_t)(result->total_mah_discharged * 100 / soc_drop);
    } else {
        result->estimated_capacity_mah = 0;
    }

    result->valid = (result->total_mah_discharged > 0 && soc_drop > 0) ? 1 : 0;
    return 0;
}

void coulomb_abort(void)
{
    g_coulomb_abort = 1;
}