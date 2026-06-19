/*
 * wheel.c — Survey Wheel Quadrature Encoder Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements a quadrature encoder interface using TIM3 on the STM32H743.
 * The survey wheel has a 1024-PPR (pulses per revolution) encoder connected
 * to TIM3_CH1 (PC6) and TIM3_CH2 (PC7).  TIM3 is configured in encoder mode
 * (count on both edges, x4 — effectively 4096 counts per revolution).
 *
 * The encoder count provides the along-track distance:
 *   distance_m = count / (PPR × 4) × wheel_circumference
 *
 * The main firmware uses TIM3's CC1 compare interrupt to trigger trace
 * acquisition at fixed distance intervals (e.g., every 20 mm = 2 cm).
 */

#include "wheel.h"
#include "../board.h"
#include "../registers.h"

/* ===================================================================== */
/*  Public API                                                            */
/* ===================================================================== */

int wheel_init(void)
{
    /* Configure TIM3 for encoder mode 3 (count on both TI1 and TI2, x4) */
    TIM3->CR1 = 0;   /* disable timer first */
    TIM3->SMCR = TIM_SMCR_SMS_ENC3;  /* encoder mode 3 */

    /* Configure channels 1 and 2 as inputs with filtering */
    TIM3->CCMR1 = (TIM_CCMR1_CC1S_TI1 << 0) | (TIM_CCMR1_CC1S_TI1 << 8);
    /* Add input filter: IC1F = 8 samples (3 << 4) on both channels */
    TIM3->CCMR1 |= (TIM_CCMR1_IC1F_8 << 4) | (TIM_CCMR1_IC1F_8 << 12);

    /* CCER: both channels input, rising edge, non-inverted */
    TIM3->CCER = 0;

    /* Set ARR to max (32-bit counter) */
    TIM3->ARR = 0xFFFFFFFF;

    /* Set prescaler to 0 (count every encoder edge) */
    TIM3->PSC = 0;

    /* Configure CC1 for compare interrupt at trace spacing interval.
     * The CCR1 value determines how many encoder counts between triggers.
     * This is set dynamically by the main firmware based on trace_spacing_mm.
     * Initialize to a default: 20 mm spacing.
     * counts_per_rev = PPR × 4 = 1024 × 4 = 4096
     * mm_per_count = circumference / 4096 = 150π / 4096 ≈ 0.115 mm/count
     * counts_for_20mm = 20 / 0.115 ≈ 174
     */
    uint32_t counts_per_trace = (uint32_t)((DEFAULT_TRACE_SPACING_MM /
                              WHEEL_CIRCUMFERENCE_MM) * WHEEL_PPR * 4);
    TIM3->CCR1 = counts_per_trace;

    /* Enable CC1 interrupt + update interrupt */
    TIM3->DIER = TIM_DIER_CC1IE | TIM_DIER_UIE;

    /* Reset counter */
    TIM3->CNT = 0;

    /* Enable TIM3 in NVIC */
    NVIC->ISER[0] = (1UL << TIM3_IRQn);

    /* Start the timer */
    TIM3->CR1 = TIM_CR1_CEN;

    return 0;
}

int32_t wheel_get_count(void)
{
    return (int32_t)TIM3->CNT;
}

float wheel_get_distance_m(void)
{
    int32_t count = (int32_t)TIM3->CNT;
    /* Distance = count / (PPR × 4) × circumference */
    float counts_per_rev = (float)(WHEEL_PPR * 4);
    float revs = (float)count / counts_per_rev;
    float dist_mm = revs * WHEEL_CIRCUMFERENCE_MM;
    return dist_mm / 1000.0f;  /* convert to meters */
}

void wheel_reset(void)
{
    TIM3->CNT = 0;
}

uint8_t wheel_check_trigger(void)
{
    /* Check if CC1 flag is set (trace spacing reached) */
    if (TIM3->SR & TIM_SR_CC1IF) {
        TIM3->SR = ~TIM_SR_CC1IF;  /* clear flag */
        return 1;
    }
    return 0;
}

/* End of wheel.c */