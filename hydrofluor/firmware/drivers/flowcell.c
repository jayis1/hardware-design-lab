/*
 * flowcell.c — HydroFluor peristaltic pump & flow cell control
 * Author: jayis1
 * License: MIT
 *
 * Drives a 6 V brushless peristaltic micro-pump via TIM2 PWM and an enable
 * MOSFET. Used for continuous-flow deployments: the pump draws sample water
 * through the blackened PTFE flow cell so the optical bench sees fresh
 * sample on every measurement cycle.
 */
#include "flowcell.h"
#include "../registers.h"

static uint8_t g_last_bubble = 0;

void flowcell_init(void)
{
    /* Enable TIM2 clock (APB1ENR1 bit 0) and GPIOB */
    RCC->APB1ENR1 |= (1U << 0);
    RCC->AHB2ENR  |= (1U << 1);

    /* PWM pin PB3 = TIM2_CH2 (AF1), enable pin PB4 (output) */
    gpio_mode(PUMP_PWM_PORT, PUMP_PWM_PIN, GPIO_MODE_AF);
    gpio_af(PUMP_PWM_PORT, PUMP_PWM_PIN, 1);
    gpio_mode(PUMP_EN_PORT, PUMP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_clr(PUMP_EN_PORT, PUMP_EN_PIN);   /* pump disabled at boot */

    /* TIM2: PCLK1 = 40 MHz, PSC = 0, ARR = 2000 → 20 kHz PWM */
    TIM2->PSC = 0;
    TIM2->ARR = 2000 - 1;
    TIM2->CCR2 = 0;                        /* duty = 0 (off) */
    TIM2->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM2->CCER = (TIM_CCER_CC1E << 4);    /* CH2 enable (bit4) */
    TIM2->CR1 |= TIM_CR1_ARPE | TIM_CR1_CEN;
}

void flowcell_pump_start(uint8_t duty_pct)
{
    if (duty_pct > 100) duty_pct = 100;
    uint32_t duty = (uint32_t)duty_pct * (TIM2->ARR + 1) / 100;
    TIM2->CCR2 = duty;
    gpio_set(PUMP_EN_PORT, PUMP_EN_PIN);
}

void flowcell_pump_stop(void)
{
    TIM2->CCR2 = 0;
    gpio_clr(PUMP_EN_PORT, PUMP_EN_PIN);
}

void flowcell_prime(uint32_t duration_ms)
{
    flowcell_pump_start(PUMP_DEFAULT_DUTY_PCT);
    /* Blocking delay — in deployment firmware this would yield to RTOS,
     * but the HydroFluor super-loop is acceptable. */
    for (volatile uint32_t i = 0; i < duration_ms * 60; ++i) { /* ~1 ms each */
        __asm__("nop");
    }
    flowcell_pump_stop();
}

uint8_t flowcell_bubble_detected(void)
{
    return g_last_bubble;
}

/* Called by fluorometry when a measurement completes — sets the bubble
 * flag based on the acquisition_t.flags bit. */
void flowcell_set_bubble_flag(uint8_t b)
{
    g_last_bubble = b ? 1 : 0;
}