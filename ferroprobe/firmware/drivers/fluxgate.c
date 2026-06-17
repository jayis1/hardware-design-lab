/*
 * fluxgate.c — Fluxgate excitation driver
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the three orthogonal fluxgate sensor cores via a shared
 * H-bridge excitation path.  TIM1 generates a center-aligned PWM at
 * 15.625 kHz with complementary outputs (CH1/CH1N, CH2/CH2N) driving
 * the TPS28225 gate driver and FDS8958A MOSFET H-bridge.  The third
 * axis uses CH3/CH3N in parallel with CH1 for additional drive
 * current capability.
 *
 * The center-aligned mode ensures symmetric positive/negative
 * excitation halves, which is critical for minimizing the
 * first-harmonic feedthrough (the even-harmonic symmetry requires
 * equal positive and negative saturation durations).
 */

#include "fluxgate.h"
#include "../board.h"
#include "../registers.h"

/* H-bridge enable pin: PD6 → TPS28225 EN */
#define HBRIDGE_EN_PORT  GPIOD_BASE
#define HBRIDGE_EN_PIN   6

void fluxgate_init(void)
{
    /* Enable GPIOE and TIM1 clocks */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
    RCC_APB2ENR |= RCC_APB2ENR_TIM1EN;

    /* Configure PE8–PE13 as TIM1 AF1 (complementary PWM outputs)
     * PE8  = TIM1_CH1N, PE9  = TIM1_CH1
     * PE10 = TIM1_CH2N, PE11 = TIM1_CH2
     * PE12 = TIM1_CH3N, PE13 = TIM1_CH3
     */
    volatile uint32_t *pe_afrl = (volatile uint32_t *)(EXC_PWM_PORT + GPIO_AFRL);
    volatile uint32_t *pe_afrh = (volatile uint32_t *)(EXC_PWM_PORT + GPIO_AFRH);
    volatile uint32_t *pe_moder = (volatile uint32_t *)(EXC_PWM_PORT + GPIO_MODER);
    volatile uint32_t *pe_ospeedr = (volatile uint32_t *)(EXC_PWM_PORT + GPIO_OSPEEDR);
    volatile uint32_t *pe_otyper = (volatile uint32_t *)(EXC_PWM_PORT + GPIO_OTYPER);

    /* Pins 8-13: AF1 (TIM1), push-pull, high speed */
    for (uint8_t pin = 8; pin <= 13; pin++) {
        uint32_t shift = (pin < 8) ? (pin * 4) : ((pin - 8) * 4);
        volatile uint32_t *afr = (pin < 8) ? pe_afrl : pe_afrh;
        /* Clear and set AF */
        *afr &= ~(0xFU << shift);
        *afr |= ((uint32_t)GPIO_AF1 << shift);
        /* Mode = AF (10) */
        *pe_moder &= ~(0x3U << (pin * 2));
        *pe_moder |= (0x2U << (pin * 2));
        /* Speed = very high (11) */
        *pe_ospeedr |= (0x3U << (pin * 2));
        /* Push-pull (0) */
        *pe_otyper &= ~(1U << pin);
    }

    /* Configure H-bridge enable pin (PD6) as output */
    RCC_AHB4ENR |= RCC_AHB4ENR_GPIODEN;
    volatile uint32_t *pd_moder = (volatile uint32_t *)(GPIOD_BASE + GPIO_MODER);
    *pd_moder &= ~(0x3U << (HBRIDGE_EN_PIN * 2));
    *pd_moder |= (0x1U << (HBRIDGE_EN_PIN * 2));  /* Output */
    gpio_reset(HBRIDGE_EN_PORT, HBRIDGE_EN_PIN);  /* H-bridge disabled */

    /* --- TIM1 configuration --- */
    TIM1_CR1 = 0;  /* Disable before configuring */
    TIM1_CR2 = 0;

    /* Prescaler: TIM1CLK = 240 MHz, PSC=0 → 240 MHz tick */
    TIM1_PSC = BOARD_TIM1_PSC;

    /* Auto-reload: center-aligned mode → period = 2 × ARR
     * ARR = 240MHz / (2 × 15625) = 7680
     */
    TIM1_ARR = BOARD_TIM1_ARR;

    /* Center-aligned mode 3 (both edges count up and down) */
    TIM1_CR1 |= TIM_CR1_CMS_CENTER;
    TIM1_CR1 |= TIM_CR1_ARPE;  /* Auto-reload preload */

    /* Channel 1: PWM mode 1, preload enable */
    TIM1_CCMR1 = 0;
    TIM1_CCMR1 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1_CCMR1 |= TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;

    /* Channel 3: PWM mode 1 (in CCMR2) */
    TIM1_CCMR2 = 0;
    TIM1_CCMR2 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;  /* OC3M = PWM1 */
    TIM1_CCMR2 |= (TIM_CCMR1_OC1M_PWM1 << 8) | (1U << 11); /* OC4M spare */

    /* Enable outputs: CH1, CH1N, CH2, CH2N, CH3, CH3N */
    TIM1_CCER = 0;
    TIM1_CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE;
    TIM1_CCER |= TIM_CCER_CC2E | TIM_CCER_CC2NE;
    TIM1_CCER |= (1U << 8) | (1U << 10);  /* CC3E + CC3NE */

    /* Dead time: 200 ns at 240 MHz → 48 counts
     * BDTR DTG[7:0] = 0x30 → ~200ns deadtime
     */
    TIM1_BDTR = 0;
    TIM1_BDTR |= (0x30U);       /* Dead time */
    TIM1_BDTR |= TIM_BDTR_MOE;  /* Main output enable */

    /* Initial duty cycle: 50% */
    TIM1_CCR1 = BOARD_TIM1_CCR;
    TIM1_CCR2 = BOARD_TIM1_CCR;
    TIM1_CCR3 = BOARD_TIM1_CCR;
    TIM1_CCR4 = 0;

    /* Generate update event to load shadow registers */
    TIM1_EGR = 1;
}

void fluxgate_enable(void)
{
    /* Enable H-bridge gate driver first */
    fluxgate_hbridge_enable();
    delay_us(100);  /* Gate driver turn-on settling */
    /* Start TIM1 counter */
    TIM1_CR1 |= TIM_CR1_CEN;
    TIM1_BDTR |= TIM_BDTR_MOE;  /* Enable main output */
}

void fluxgate_disable(void)
{
    TIM1_BDTR &= ~TIM_BDTR_MOE;  /* Disable main output */
    TIM1_CR1 &= ~TIM_CR1_CEN;    /* Stop counter */
    delay_us(50);
    fluxgate_hbridge_disable();
}

void fluxgate_set_amplitude(float fraction)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 0.9f) fraction = 0.9f;  /* Never fully saturate H-bridge */

    uint32_t ccr = (uint32_t)(fraction * (float)BOARD_TIM1_ARR);
    TIM1_CCR1 = ccr;
    TIM1_CCR2 = ccr;
    TIM1_CCR3 = ccr;
}

uint32_t fluxgate_get_frequency(void)
{
    return BOARD_EXCIT_FREQ_HZ;
}

uint16_t fluxgate_get_2f0_phase(void)
{
    /* The 2f0 reference is derived from the TIM1 update event,
     * which occurs at 2× the excitation frequency (center-aligned).
     * Phase = (CNT / ARR) × 180° → 2f0 phase wraps every ARR/2 counts.
     */
    uint32_t cnt = TIM1_CNT;
    if (cnt > BOARD_TIM1_ARR)
        cnt = BOARD_TIM1_ARR;
    return (uint16_t)((cnt * 180U) / BOARD_TIM1_ARR);
}

void fluxgate_hbridge_enable(void)
{
    gpio_set(HBRIDGE_EN_PORT, HBRIDGE_EN_PIN);
}

void fluxgate_hbridge_disable(void)
{
    gpio_reset(HBRIDGE_EN_PORT, HBRIDGE_EN_PIN);
}