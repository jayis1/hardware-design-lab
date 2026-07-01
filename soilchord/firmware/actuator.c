/*
 * actuator.c — Soilchord solenoid pluck driver
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Drives 4 solenoids (mapped to 6 chords) via TIM1 PWM-gated MOSFETs.
 * Each "pluck" is a 30 ms current pulse through the appropriate channel.
 */
#include "soilchord.h"
#include "registers.h"
#include "board.h"

/* We model the GPIO/solenoid control via simple register writes. In the
 * reference build these are the TIM1 CCR registers; here we abstract them
 * behind a per-driver enable/disable so the driver is readable. */

typedef struct {
    volatile uint32_t *ccr;     /* TIM1 CCRx for this driver */
} sol_driver_t;

static sol_driver_t s_solenoids[4] = {
    { (volatile uint32_t *)(TIM1_BASE_REG + 0x34) },   /* CCR1 */
    { (volatile uint32_t *)(TIM1_BASE_REG + 0x38) },   /* CCR2 */
    { (volatile uint32_t *)(TIM1_BASE_REG + 0x3C) },   /* CCR3 */
    { (volatile uint32_t *)(TIM1_BASE_REG + 0x40) },   /* CCR4 */
};

static void delay_ms_busy(uint32_t ms)
{
    /* Rough busy-wait at 80 MHz: ~80000 cycles per ms. */
    for (uint32_t i = 0; i < ms * 8000; i++) {
        __asm__("nop");
    }
}

void actuator_init(void)
{
    /* Enable TIM1 clock. */
    RCC_APB2ENR |= (1U << 11);           /* TIM1EN */

    /* Configure the four PWM channels as outputs with a 1 kHz base frequency.
     * We use the timer purely as a timed pulse generator: ARR=80 gives 1 kHz,
     * and we load CCR=80 for the "on" duration and CCR=0 for "off". */
    volatile uint32_t *tim1_cr1 = (volatile uint32_t *)(TIM1_BASE_REG + 0x00);
    volatile uint32_t *tim1_arr = (volatile uint32_t *)(TIM1_BASE_REG + 0x2C);
    volatile uint32_t *tim1_psc = (volatile uint32_t *)(TIM1_BASE_REG + 0x28);
    volatile uint32_t *tim1_ccer = (volatile uint32_t *)(TIM1_BASE_REG + 0x20);
    volatile uint32_t *tim1_ccmr1 = (volatile uint32_t *)(TIM1_BASE_REG + 0x18);
    volatile uint32_t *tim1_ccmr2 = (volatile uint32_t *)(TIM1_BASE_REG + 0x1C);
    volatile uint32_t *tim1_bdtr = (volatile uint32_t *)(TIM1_BASE_REG + 0x44);
    volatile uint32_t *tim1_egr = (volatile uint32_t *)(TIM1_BASE_REG + 0x14);

    *tim1_psc = 0;                       /* 80 MHz timer clock */
    *tim1_arr = 80;                       /* 1 kHz PWM */
    /* PWM mode 1 on all 4 channels, preload enabled. */
    *tim1_ccmr1 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    *tim1_ccmr2 = (6U << 4) | (1U << 3) | (6U << 12) | (1U << 11);
    *tim1_ccer  = (1U << 0) | (1U << 4) | (1U << 8) | (1U << 12);
    *tim1_bdtr  = (1U << 15);             /* MOE — main output enable */
    *tim1_egr   = 1U;                     /* UG — force update */

    /* All channels idle (off). */
    for (int i = 0; i < 4; i++) *s_solenoids[i].ccr = 0;

    *tim1_cr1 = 1U;                       /* CEN — enable counter */
}

sc_err_t actuator_pluck(uint8_t chord)
{
    if (chord >= NCHORDS) return SC_ERR_RANGE;
    uint8_t sol = kSolenoidForChord[chord];
    if (sol >= 4) return SC_ERR_RANGE;

    /* Drive the solenoid at ~100% duty for PLUCK_MS, then release. */
    *s_solenoids[sol].ccr = 80;           /* full on */
    delay_ms_busy(PLUCK_MS);
    *s_solenoids[sol].ccr = 0;            /* off */

    /* Ensure the lever has fully returned before the next pluck. */
    delay_ms_busy(10);
    return SC_OK;
}