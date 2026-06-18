/*
 * leafwet.c — Leaf-wetness capacitive sensor driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Measures leaf surface wetness using a capacitive sensor: a thin PCB
 * interdigitated electrode laminate pressed against the leaf surface.
 * The sensor forms the timing capacitor in a CMOS 555 oscillator, and
 * the oscillator frequency (measured via TIM3 external clock input on
 * PD2/TIM3_ETR) indicates wetness.
 *
 *  - Dry leaf: C ≈ 100 pF → f ≈ 10 kHz
 *  - Wet leaf: C ≈ 200 pF → f ≈ 5 kHz
 *  - Water film: C ≈ 500 pF → f ≈ 2 kHz
 *
 * The measurement is gated for 100 ms (BOARD_LW_GATE_MS) to get
 * sufficient count resolution (1000 counts at 10 kHz).
 */

#include "leafwet.h"
#include "../board.h"
#include "../registers.h"

static uint16_t last_freq_hz = 0;

/* ===================================================================== */
/*  Initialization                                                       */
/* ===================================================================== */

void leafwet_init(void)
{
    /* Enable GPIOD and TIM3 clocks */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIODEN | RCC_AHB2ENR_GPIOCEN;
    RCC_APB1ENR1 |= RCC_APB1ENR1_TIM3EN;

    /* PD2 as AF2 (TIM3_ETR on L4) */
    volatile uint32_t *pd_moder = (volatile uint32_t *)(GPIOD_BASE + GPIO_MODER);
    volatile uint32_t *pd_afrl  = (volatile uint32_t *)(GPIOD_BASE + GPIO_AFRL);
    volatile uint32_t *pd_pupdr = (volatile uint32_t *)(GPIOD_BASE + GPIO_PUPDR);

    *pd_moder &= ~(0x3U << (LW_ETR_PIN * 2));
    *pd_moder |= (0x2U << (LW_ETR_PIN * 2));   /* AF mode */
    *pd_afrl &= ~(0xFU << (LW_ETR_PIN * 4));
    *pd_afrl |= ((uint32_t)0x2U << (LW_ETR_PIN * 4));  /* AF2 */
    *pd_pupdr &= ~(0x3U << (LW_ETR_PIN * 2));  /* No pull */

    /* PC2: sensor drive enable (output) */
    volatile uint32_t *pc_moder = (volatile uint32_t *)(GPIOC_BASE + GPIO_MODER);
    *pc_moder |= (0x1U << (LW_DRIVE_PIN * 2));
    gpio_reset(LW_DRIVE_PORT, LW_DRIVE_PIN);

    /* TIM3: external clock mode 2 (ETR pin → counter clock)
     * SMCR: ETF=0000 (no filter), ETPS=00 (no prescaler), ECE=1
     */
    TIM3_CR1 = 0;
    TIM3_SMCR = 0;
    TIM3_SMCR |= (1U << 14);   /* ECE: external clock mode 2 */
    TIM3_PSC = 0;
    TIM3_ARR = 0xFFFFU;        /* 16-bit max count */
}

/* ===================================================================== */
/*  Measurement                                                          */
/* ===================================================================== */

uint16_t leafwet_measure_hz(void)
{
    /* Enable sensor oscillator */
    gpio_set(LW_DRIVE_PORT, LW_DRIVE_PIN);

    /* Reset counter */
    TIM3_CNT = 0;
    TIM3_CR1 = TIM_CR1_CEN;   /* Start counting */

    /* Gate for 100 ms */
    /* At 80 MHz, a busy-loop of ~8M iterations ≈ 100 ms.
     * For accuracy, we use TIM6 for the gate, but simplified here.
     */
    for (volatile uint32_t i = 0; i < 8000000; i++)
        ;

    /* Stop counter */
    TIM3_CR1 = 0;

    /* Disable sensor */
    gpio_reset(LW_DRIVE_PORT, LW_DRIVE_PIN);

    /* Read count.  Frequency = count / gate_time = count / 0.1 s = count × 10 */
    uint16_t count = (uint16_t)TIM3_CNT;
    last_freq_hz = count * 10U;   /* Hz */

    return last_freq_hz;
}

uint8_t leafwet_to_pct(uint16_t freq_hz)
{
    /* Linear interpolation:
     * 10 kHz = 0% wet (dry)
     * 5 kHz   = 100% wet (saturated)
     * Below 5 kHz = 100% (clamped)
     * Above 10 kHz = 0% (clamped)
     */
    if (freq_hz >= BOARD_LW_FREQ_DRY_HZ)
        return 0;
    if (freq_hz <= BOARD_LW_FREQ_WET_HZ)
        return 100;

    /* Linear: pct = (dry - freq) / (dry - wet) × 100 */
    uint32_t pct = ((uint32_t)(BOARD_LW_FREQ_DRY_HZ - freq_hz) * 100U) /
                   (BOARD_LW_FREQ_DRY_HZ - BOARD_LW_FREQ_WET_HZ);
    return (uint8_t)pct;
}

uint16_t leafwet_get_last_hz(void)
{
    return last_freq_hz;
}