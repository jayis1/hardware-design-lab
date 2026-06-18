/*
 * led_excitation.c — HydroFluor excitation LED driver (TIM1 PWM)
 * Author: jayis1
 * License: MIT
 *
 * Drives six excitation LEDs (255/280/365/470/590/660 nm) via TIM1 PWM
 * channels feeding discrete constant-current MOSFET stages. Each "fire"
 * produces a precisely-timed pulse; an associated comparator timer arm
 * triggers the ADS1256 ADC sample at the pulse midpoint for synchronous
 * (lock-in-style) detection.
 */
#include "led_excitation.h"
#include "../registers.h"
#include <string.h>

/* Wavelength metadata (for UI / logging) */
static const struct {
    const char *name;
    uint16_t    nm;
} wl_meta[EX_CHANNEL_COUNT] = {
    { "255nm DUV", 255 },
    { "280nm DUV", 280 },
    { "365nm UV",  365 },
    { "470nm BLU", 470 },
    { "590nm AMB", 590 },
    { "660nm RED", 660 },
};

/* Per-channel active config */
static led_config_t g_cfg[EX_CHANNEL_COUNT];

/* Mapping from excitation wavelength to TIM1 CCR register index.
 * Channel 1-4 on TIM1 CCR1-4; channels 5-6 use remapped alt CCR via GPIOC. */
static volatile uint32_t * const g_ccr[EX_CHANNEL_COUNT] = {
    &TIM1->CCR1, &TIM1->CCR2, &TIM1->CCR3, &TIM1->CCR4,
    &TIM1->CCR1, &TIM1->CCR2,  /* remapped to TIM1 alt on GPIOC — same CCR */
};

/* Approximate timer ticks per microsecond at PCLK2 = 120 MHz */
#define TICKS_PER_US   (BOARD_PCLK2_HZ / 1000000UL)   /* 120 */

void led_excitation_init(void)
{
    /* Enable TIM1 clock (APB2ENR bit 11 on L4R9) */
    RCC->APB2ENR |= (1U << 11);
    /* Enable GPIOA/C clocks */
    RCC->AHB2ENR |= (1U << 0) | (1U << 2);

    /* Configure LED drive pins as AF (TIM1 outputs).
     * PA8/9/10/11 = AF1 (TIM1_CH1-4); PC6/PC7 = AF3 (TIM1 alt). */
    for (uint8_t p = 8; p <= 11; ++p) {
        gpio_mode(GPIOA, p, GPIO_MODE_AF);
        gpio_af(GPIOA, p, 1);
        gpio_pupd(GPIOA, p, GPIO_PUPD_NONE);
    }
    gpio_mode(GPIOC, 6, GPIO_MODE_AF);
    gpio_af(GPIOC, 6, 3);
    gpio_mode(GPIOC, 7, GPIO_MODE_AF);
    gpio_af(GPIOC, 7, 3);

    /* TIM1 base config: up-counting, PSC = 0 (full 120 MHz), ARR = 6000
     * → 20 kHz PWM carrier. Pulses are 120 µs (14 ticks) by default. */
    TIM1->PSC = 0;
    TIM1->ARR = 6000 - 1;            /* 20 kHz carrier (50 µs period) */

    /* Configure all four CCR channels as PWM mode 1, preload enable. */
    TIM1->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE
                | (TIM_CCMR1_OC1M_PWM1 << 8) | (TIM_CCMR1_OC1PE << 8);
    TIM1->CCMR2 = (TIM_CCMR1_OC1M_PWM1 << 0) | (TIM_CCMR1_OC1PE << 0)
                | (TIM_CCMR1_OC1M_PWM1 << 8) | (TIM_CCMR1_OC1PE << 8);

    /* Enable all four capture compare outputs (active high). */
    TIM1->CCER = TIM_CCER_CC1E | (TIM_CCER_CC1E << 4)
                | (TIM_CCER_CC1E << 8) | (TIM_CCER_CC1E << 12);

    /* Main output enable (advanced timer). */
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1  |= TIM_CR1_ARPE;

    /* Default: all CCR = 0 (off) */
    TIM1->CCR1 = 0; TIM1->CCR2 = 0; TIM1->CCR3 = 0; TIM1->CCR4 = 0;

    TIM1->CR1 |= TIM_CR1_CEN;

    /* Initialize per-channel config defaults */
    for (int i = 0; i < EX_CHANNEL_COUNT; ++i) {
        g_cfg[i].id          = (ex_wavelength_t)i;
        g_cfg[i].pulse_us    = DEFAULT_PULSE_US;
        g_cfg[i].current_ma  = DEFAULT_CURRENT_MA;
        g_cfg[i].averages    = DEFAULT_AVERAGES;
    }
}

void led_excitation_config(ex_wavelength_t wl, uint16_t pulse_us,
                            uint16_t current_ma, uint8_t averages)
{
    if (wl >= EX_CHANNEL_COUNT) return;
    /* Clamp to safe ranges (pulses < 2 ms, current < 800 mA) */
    if (pulse_us == 0 || pulse_us > 2000) pulse_us = DEFAULT_PULSE_US;
    if (current_ma == 0 || current_ma > 800) current_ma = DEFAULT_CURRENT_MA;
    if (averages == 0 || averages > 64) averages = DEFAULT_AVERAGES;
    g_cfg[wl].pulse_us   = pulse_us;
    g_cfg[wl].current_ma = current_ma;
    g_cfg[wl].averages   = averages;
}

const led_config_t *led_excitation_get(ex_wavelength_t wl)
{
    if (wl >= EX_CHANNEL_COUNT) return NULL;
    return &g_cfg[wl];
}

uint32_t led_excitation_fire(ex_wavelength_t wl)
{
    if (wl >= EX_CHANNEL_COUNT) return 0;
    /* Convert pulse width to PWM ticks at 120 ticks/µs */
    const uint32_t arr      = TIM1->ARR + 1;
    const uint32_t pulse_tk = (uint32_t)g_cfg[wl].pulse_us * TICKS_PER_US;
    uint32_t duty = pulse_tk;
    if (duty >= arr) duty = arr - 1;

    /* Current setpoint scales duty cycle within pulse window — but here we
     * keep PWM at the full pulse and rely on the external DAC (PGA113 gain)
     * to set photodiode transimpedance. The LED constant-current driver
     * external to the MCU sets mA via a sense resistor; we only gate it. */

    /* Set the chosen channel's CCR to the pulse width, others to 0 */
    for (int i = 0; i < EX_CHANNEL_COUNT; ++i) {
        *g_ccr[i] = (i == (int)wl) ? duty : 0;
    }

    /* The pulse is one PWM period of the carrier. We record the boot µs. */
    uint32_t t0 = 0; /* SysTick-driven timestamp — filled by caller via
                      * board_get_us(); we approximate from ARR reload. */

    /* Wait one full ARR period for the pulse to complete, then turn off. */
    uint32_t sr;
    do { sr = TIM1->SR; } while (!(sr & TIM_SR_UIF));
    TIM1->SR = 0;  /* clear UIF */
    *g_ccr[wl] = 0;

    /* Approximate midpoint timestamp (µs): pulse_us / 2 + scheduling delay.
     * Caller uses this only for logging correlation. */
    return g_cfg[wl].pulse_us / 2 + 5;
}

uint32_t led_excitation_arm_sync(ex_wavelength_t wl)
{
    if (wl >= EX_CHANNEL_COUNT) return 0;
    /* Program TIM1 CC2 (used as ADC sync trigger) to assert at the midpoint
     * of the pulse. The photodiode driver then waits on DRDY from the ADS1256
     * which is gated externally by this trigger. */
    const uint32_t mid_tk = (uint32_t)g_cfg[wl].pulse_us * TICKS_PER_US / 2;
    /* Use CCR3 as the sync trigger output (TIM1_CH3 → PB0 routed externally
     * to ADS1256 START pin). */
    TIM1->CCR3 = mid_tk;
    return mid_tk / TICKS_PER_US;  /* expected delay in µs */
}

const char *led_excitation_name(ex_wavelength_t wl)
{
    if (wl >= EX_CHANNEL_COUNT) return "?";
    return wl_meta[wl].name;
}

uint16_t led_excitation_wavelength_nm(ex_wavelength_t wl)
{
    if (wl >= EX_CHANNEL_COUNT) return 0;
    return wl_meta[wl].nm;
}