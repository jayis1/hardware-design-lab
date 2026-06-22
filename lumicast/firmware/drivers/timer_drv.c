/*
 * timer_drv.c — System timer & delay driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Provides millisecond and microsecond delays using the Cortex-M4 SysTick
 * timer running at 64 MHz, plus a monotonically increasing millisecond
 * counter updated from the SysTick interrupt.
 */

#include "timer_drv.h"

static volatile uint32_t ms_counter = 0;
static volatile uint32_t overflow_ticks = 0;

void timer_init(void)
{
    /* Configure SysTick for 1 ms tick at 64 MHz. */
    SYST_RVR = SYSTICK_RELOAD;
    SYST_CVR = 0;
    ms_counter = 0;
    overflow_ticks = 0;
    SYST_CSR = 0x7U;  /* ENABLE | TICKINT | CLKSOURCE(core) */
}

void timer_delay_ms(uint32_t ms)
{
    uint32_t start = ms_counter;
    while ((ms_counter - start) < ms) {
        __asm__ volatile ("wfi");
    }
}

void timer_delay_us(uint32_t us)
{
    /* Coarse microsecond delay using SysTick counter. */
    uint32_t cycles = us * (SYSTEM_CLK_HZ / 1000000UL);
    uint32_t start = SYST_CVR;
    uint32_t elapsed;
    do {
        uint32_t now = SYST_CVR;
        elapsed = (start > now) ? (start - now) : (start + (SYSTICK_RELOAD + 1 - now));
    } while (elapsed < cycles);
}

uint32_t timer_millis(void) { return ms_counter; }

uint32_t timer_micros(void)
{
    uint32_t ms = ms_counter;
    uint32_t cvr = SYST_CVR;
    uint32_t us_per_tick = 1000000UL / SYST_HZ;
    return (ms * 1000UL) + ((SYSTICK_RELOAD - cvr) * us_per_tick);
}

bool timer_elapsed(uint32_t start_ms, uint32_t interval_ms)
{
    return (ms_counter - start_ms) >= interval_ms;
}

/* SysTick interrupt handler. */
void SysTick_Handler(void)
{
    ms_counter++;
    if (ms_counter == 0) overflow_ticks++;
}