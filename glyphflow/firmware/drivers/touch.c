/*
 * touch.c — Capacitive button sense (RC discharge timing)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * The wristband's "pen" button is a 12 mm copper pad on the inner side of
 * the band, connected to PA8 with a 1 MΩ pull-up. We charge the pad high,
 * then reconfigure PA8 as input and count SysTick ticks until it reads low.
 * A finger touch adds ~20 pF and stretches the discharge from ~3 µs to
 * ~80 µs. We threshold at 40 ticks. This is a deliberately simple, no-extra-
 * hardware approach; the STM32L432's built-in touch sense controller is
 * not used to keep the register header minimal.
 */
#include "touch.h"
#include "../registers.h"
#include "../board.h"

#define TOUCH_THRESHOLD_TICKS  40

static volatile uint8_t g_last_state = 0;
static volatile uint8_t g_edge = 0;

int touch_init(void)
{
    /* Start with PA8 as input. */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_TOUCH_BIT*2)));
    g_last_state = 0;
    return 0;
}

uint8_t touch_read(void)
{
    /* Charge: drive PA8 high for ~5 µs. */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_TOUCH_BIT*2)))
                 | (GPIO_MODE_OUT << (PIN_TOUCH_BIT*2));
    GPIO_SET(PIN_TOUCH_PORT, PIN_TOUCH_BIT);
    for (volatile int i = 0; i < 40; i++) { }   /* ~5 µs at 80 MHz */

    /* Float: PA8 as input. */
    GPIOA->MODER = (GPIOA->MODER & ~(3U << (PIN_TOUCH_BIT*2)));

    /* Count ticks until PA8 reads 0. */
    uint32_t start = DWT_CYCCNT;
    uint32_t max_cycles = 2000;   /* 25 µs cap */
    while (GPIO_GET(PIN_TOUCH_PORT, PIN_TOUCH_BIT)) {
        if ((DWT_CYCCNT - start) > max_cycles) break;
    }
    uint32_t elapsed = DWT_CYCCNT - start;
    /* elapsed is in CPU cycles; at 80 MHz, 40 ticks ≈ 0.5 µs is too short —
     * we actually want microseconds, so divide by 80. */
    uint32_t us = elapsed / 80;
    return (us >= 5) ? 1 : 0;   /* ≥5 µs discharge = touched */
}

uint8_t touch_debounce(void)
{
    uint8_t raw = touch_read();
    static uint8_t stable = 0;
    static uint8_t count = 0;
    if (raw == stable) {
        count = 0;
    } else {
        if (++count >= 3) {   /* 3 consecutive reads → accept change */
            stable = raw;
            if (stable) g_edge = 1;  /* rising edge = press */
        }
    }
    uint8_t e = g_edge;
    g_edge = 0;
    return e;
}

void touch_reset(void) { g_last_state = 0; g_edge = 0; }