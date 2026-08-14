/*
 * drivers/mux.c — TMUX1108 8:1 analog multiplexer driver
 *
 * The TMUX1108 is a precision low-leakage CMOS analog mux.  Its 3
 * address pins (A0/A1/A2) select one of 8 inputs (S1..S8) connected to
 * the output (D), which feeds the ADC's HALL channel.  The enable pin
 * (/EN) is active low; when high, the mux outputs high-impedance.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "../board.h"
#include "../registers.h"
#include "mux.h"

void mux_init(void)
{
    /* Enable GPIOA clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC_AHB2ENR;

    /* Configure PA4/PA5/PA6/PA7 as push-pull output, low speed */
    volatile uint32_t * moder = &GPIO_MODER(GPIOA_BASE);
    *moder &= ~((3u << (MUX_A0_PIN * 2u)) |
               (3u << (MUX_A1_PIN * 2u)) |
               (3u << (MUX_A2_PIN * 2u)) |
               (3u << (MUX_EN_PIN * 2u)));
    *moder |=  ((GPIO_MODE_OUTPUT << (MUX_A0_PIN * 2u)) |
               (GPIO_MODE_OUTPUT << (MUX_A1_PIN * 2u)) |
               (GPIO_MODE_OUTPUT << (MUX_A2_PIN * 2u)) |
               (GPIO_MODE_OUTPUT << (MUX_EN_PIN * 2u)));

    /* Push-pull, low-speed, no pull */
    GPIO_OSPEEDR(GPIOA_BASE) &= ~((3u << (MUX_A0_PIN * 2u)) |
                                  (3u << (MUX_A1_PIN * 2u)) |
                                  (3u << (MUX_A2_PIN * 2u)) |
                                  (3u << (MUX_EN_PIN * 2u)));
    GPIO_PUPDR(GPIOA_BASE) &= ~((3u << (MUX_A0_PIN * 2u)) |
                                 (3u << (MUX_A1_PIN * 2u)) |
                                 (3u << (MUX_A2_PIN * 2u)) |
                                 (3u << (MUX_EN_PIN * 2u)));

    /* Start with mux disabled (EN = 1, active-low) */
    GPIO_BSRR(GPIOA_BASE) = (1u << (MUX_EN_PIN + 16u)); /* reset = high */
    mux_disable();
}

/*
 * Select one of 8 channels (0..7).  Also asserts /EN (low) to enable
 * the mux output.  A settle delay of 2 us is inserted to allow the
 * mux output to settle to within 0.1%.
 */
void mux_select(uint8_t channel)
{
    channel &= 0x07u;

    /* A0 = bit0, A1 = bit1, A2 = bit2 */
    if (channel & 0x1u)
        GPIO_BSRR(GPIOA_BASE) = (1u << MUX_A0_PIN);       /* set */
    else
        GPIO_BSRR(GPIOA_BASE) = (1u << (MUX_A0_PIN + 16u));/* reset */
    if (channel & 0x2u)
        GPIO_BSRR(GPIOA_BASE) = (1u << MUX_A1_PIN);
    else
        GPIO_BSRR(GPIOA_BASE) = (1u << (MUX_A1_PIN + 16u));
    if (channel & 0x4u)
        GPIO_BSRR(GPIOA_BASE) = (1u << MUX_A2_PIN);
    else
        GPIO_BSRR(GPIOA_BASE) = (1u << (MUX_A2_PIN + 16u));

    /* Enable mux output (active-low EN -> drive low) */
    GPIO_BSRR(GPIOA_BASE) = (1u << (MUX_EN_PIN + 16u)); /* EN = 0 */

    /* ~2 us settle for TMUX1108 (t_on max 350 ns + RC of filter) */
    for (volatile uint32_t i = 0; i < 40u; i++) { __asm__("nop"); }
}

void mux_disable(void)
{
    /* EN high -> mux output high-Z (saves ~ 80 uA per channel) */
    GPIO_BSRR(GPIOA_BASE) = (1u << MUX_EN_PIN); /* EN = 1 */
}