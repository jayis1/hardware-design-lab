/*
 * drivers/leafwet.c — Capacitive leaf wetness sensor driver
 *
 * A NE555 timer configured as an astable multivibrator converts the
 * capacitance of the interdigitated leaf-wetness plate into a frequency.
 * The MCU captures this frequency using TIM3 channel 1 in input-capture
 * mode over a 100 ms gate window.
 *
 * Dry plate: ~18 pF → ~50 kHz
 * Fully wet: ~45 pF → ~20 kHz
 *
 * The frequency is mapped to a normalized wetness index 0–1000 where
 * 0 = bone dry and 1000 = saturated film.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "leafwet.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  TIM3 CH1 input capture configuration                               */
/*  PSC = (160 MHz / 1 MHz) - 1 = 159  →  timer clock 1 MHz (1 µs)    */
/*  ARR = 0xFFFF                                                        */
/*  CCMR1: CC1 mapped to TI1, filtered, edge detector                   */
/* ------------------------------------------------------------------ */
static void tim3_ic_init(void)
{
    /* Enable TIM3 clock */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;

    /* Configure PB4 as TIM3_CH1 alternate function (AF2) */
    GPIO_CONFIG(GPIOB, 4, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH,
                GPIO_PUPD_NONE, 2);

    TIM3->PSC = 159;        /* 1 MHz timer clock */
    TIM3->ARR = 0xFFFF;
    TIM3->CCMR1 = TIM_CCMR1_IC1_INPUT | (3u << 4);  /* IC1 filtered, prescaler 1 */
    TIM3->CCER = 0x0B;      /* CC1P=1, CC1NP=1, CC1E=1 (both edges) */
    TIM3->CR1 = TIM_CR1_CEN;
}

/* ------------------------------------------------------------------ */
/*  Measure frequency over a 100 ms gate                               */
/*  Returns frequency in Hz, or 0 on error.                            */
/* ------------------------------------------------------------------ */
static uint32_t measure_freq_hz(void)
{
    uint32_t freq = 0;

    /* Reset timer */
    TIM3->CNT = 0;
    uint32_t start = time_ms();
    uint32_t first_capture = 0, last_capture = 0;
    uint32_t edges = 0;

    /* Gate for 100 ms */
    while (elapsed_ms(start) < 100) {
        if (TIM3->SR & TIM_SR_CC1IF) {
            uint32_t cap = TIM3->CCR1;
            TIM3->SR = 0;  /* clear flag */
            if (edges == 0) first_capture = cap;
            last_capture = cap;
            edges++;
        }
    }

    if (edges < 2) return 0;

    /* Frequency = (edges - 1) / (gate_seconds) */
    /* gate = 0.1 s, so freq = (edges - 1) * 10 */
    freq = (edges - 1) * 10;

    /* Sanity: validate against capture delta */
    uint32_t period_us = last_capture - first_capture;
    if (period_us > 0 && edges > 1) {
        uint32_t freq_from_cap = ((edges - 1) * 1000000u) / period_us;
        /* Average the two methods for robustness */
        freq = (freq + freq_from_cap) / 2;
    }

    return freq;
}

/* ------------------------------------------------------------------ */
/*  Map frequency to normalized wetness (0–1000)                       */
/*                                                                    */
/*  Calibration (site-configurable, stored in g_sys):                  */
/*    freq_dry  = 50000 Hz (default)                                   */
/*    freq_wet  = 20000 Hz (default)                                   */
/*                                                                    */
/*  wetness = 1000 * (freq_dry - freq) / (freq_dry - freq_wet)        */
/*  Clamped to [0, 1000].                                             */
/* ------------------------------------------------------------------ */
static uint16_t freq_to_wetness(uint32_t freq)
{
    /* Default calibration; in production these come from flash config */
    const uint32_t freq_dry = 50000;
    const uint32_t freq_wet = 20000;
    const uint32_t span = freq_dry - freq_wet;

    if (span == 0) return 0;
    if (freq >= freq_dry) return 0;
    if (freq <= freq_wet) return 1000;

    int32_t w = (int32_t)((freq_dry - freq) * 1000u) / span;
    if (w < 0) w = 0;
    if (w > 1000) w = 1000;
    return (uint16_t)w;
}

/* ------------------------------------------------------------------ */
/*  Public: read leaf wetness                                          */
/*  Returns normalized wetness 0–1000.                                 */
/* ------------------------------------------------------------------ */
int leafwet_read(uint16_t *wetness_out)
{
    static uint8_t s_init = 0;
    if (!s_init) {
        tim3_ic_init();
        s_init = 1;
        delay_ms(1);
    }

    uint32_t freq = measure_freq_hz();
    if (freq == 0) {
        *wetness_out = 0;
        return -1;
    }

    *wetness_out = freq_to_wetness(freq);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Dew detection: returns 1 if wetness exceeds the dew threshold.     */
/* ------------------------------------------------------------------ */
int leafwet_is_dew_present(uint16_t wetness)
{
    return (wetness >= (uint16_t)LEAF_WET_DEW_THRESHOLD) ? 1 : 0;
}