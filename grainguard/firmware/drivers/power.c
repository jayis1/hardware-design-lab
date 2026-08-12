/*
 * power.c — Power management & RTC scheduler
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Manages the STM32WL55 low-power modes and the RTC wake-up timer.
 * The device spends >99% of its life in STOP2 (1.2 µA), waking every
 * 15 minutes (T/RH/CO2 cycle) or 6 hours (acoustic scan).
 */

#include "power.h"
#include "../board.h"
#include "../registers.h"

static int woke_from_stop = 0;

/* ---- ADC for battery reading ---- */
static uint16_t adc_read_channel(uint8_t channel) {
    /* Enable ADC clock */
    ADC1->CFGR = 0;
    ADC1->SQR1 = (1 << 29) | (channel << 6);
    ADC1->SMPR1 = (7 << (channel * 3));
    ADC1->ISR = ADC_ISR_ADRDY;
    ADC1->CR |= ADC_CR_ADEN;
    while (!(ADC1->ISR & ADC_ISR_ADRDY)) { }
    ADC1->CR |= ADC_CR_ADSTART;
    while (!(ADC1->ISR & ADC_ISR_EOC)) { }
    return (uint16_t)ADC1->DR;
}

/* ---- Public API ---- */

int power_init(void) {
    /* Enable clocks for RTC, PWR, ADC */
    /* Enable GPIO ports */
    /* (Simplified: in real BSP these are in RCC->AHBENR / APBENR) */

    /* Configure PB9 (BAT_DIV) as analog ADC input */
    GPIOB->MODER = (GPIOB->MODER & ~(0x3 << (PB9__BAT_DIV * 2)))
                  | (GPIO_MODE_ANALOG << (PB9__BAT_DIV * 2));

    /* Configure PC4 (SUPER_CAP_SENSE) as analog */
    GPIOC->MODER = (GPIOC->MODER & ~(0x3 << (PC4__SUPER_CAP_SENSE * 2)))
                  | (GPIO_MODE_ANALOG << (PC4__SUPER_CAP_SENSE * 2));

    /* Enable PWR clock and configure LSE drive for RTC */
    /* Disable write protection on RTC registers */
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* If not initialized, enter initialization mode */
    if (!(RTC->ISR & RTC_ISR_INIT)) {
        RTC->ISR = RTC_ISR_INIT;
        while (!(RTC->ISR & RTC_ISR_INIT)) { }
        /* Configure prescalers for 1 Hz from 32768 Hz LSE */
        RTC->PRER = (127 << 16) | 255;
        RTC->ISR = 0;  /* exit init */
    }

    /* Configure wake-up clock to RTC/16 (2048 Hz) */
    RTC->CR &= ~RTC_CR_WUTE;   /* disable WUT */
    while (!(RTC->ISR & (1 << 2))) { }  /* WUTWF */
    RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL_MASK) | RTC_CR_WUCKSEL_RTCDIV16;

    woke_from_stop = 0;
    return 0;
}

uint16_t power_read_battery_mv(void) {
    uint16_t raw = adc_read_channel(PB9__BAT_DIV);
    /* 12-bit ADC, VREF ~3.0V, divider ratio 2
     * voltage = raw / 4095 * 3000 * VBAT_DIVIDER_RATIO */
    uint32_t mv = (uint32_t)raw * 3000 * VBAT_DIVIDER_RATIO / 4095;
    return (uint16_t)mv;
}

uint16_t power_read_supercap_mv(void) {
    uint16_t raw = adc_read_channel(PC4__SUPER_CAP_SENSE);
    uint32_t mv = (uint32_t)raw * 3000 / 4095;
    return (uint16_t)mv;
}

void power_schedule_wakeup(uint32_t seconds) {
    RTC->CR &= ~RTC_CR_WUTE;
    while (!(RTC->ISR & (1 << 2))) { }
    /* WUT counter: at 2048 Hz, ticks = seconds * 2048 */
    uint32_t ticks = seconds * 2048;
    if (ticks > 0xFFFF) ticks = 0xFFFF;
    RTC->WUTR = (uint16_t)ticks;
    /* Clear wake-up flag */
    RTC->ISR = 0;
    /* Enable wake-up timer + interrupt */
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
}

void power_enter_stop2(uint32_t sleep_seconds) {
    power_schedule_wakeup(sleep_seconds);

    /* Clear any pending wake-up flag */
    RTC->ISR = 0;

    /* Enter STOP2 mode (set SLEEPDEEP + PDDS bits in PWR + SCB) */
    PWR->CR3 &= ~(0x3 << 10);  /* clear LSEDRV for low power */
    PWR->CR1 = 0;  /* select STOP2 (not STANDBY) */

    /* Set SLEEPDEEP in SCB->SCR */
    *(volatile uint32_t *)0xE000ED10 |= (1 << 2);

    /* WFI instruction enters STOP2 */
    __asm volatile("wfi");

    /* ---- Woke up here ---- */
    woke_from_stop = 1;

    /* Re-enable clocks (they're off in STOP2) */
    /* (In real BSP: re-enable HSI, PLL, peripherals) */
    delay_ms(5);
}

int power_woke_from_stop(void) {
    return woke_from_stop;
}

uint32_t rtc_get_epoch_seconds(void) {
    /* Read TR (time) and DR (date) and convert to epoch seconds.
     * For simplicity, return a monotonic counter; in a full impl,
     * we'd parse BCD time/date. */
    /* Use wake-up timer count as a coarse epoch if not set. */
    static uint32_t epoch_base = 0;
    if (epoch_base == 0) {
        /* First call: set a reasonable default (could be set via NFC/app) */
        epoch_base = 1735689600;  /* 2025-01-01 00:00 UTC */
    }
    /* In a real impl, read TR+DR and compute.  Here we approximate
     * using a counter incremented on each wake. */
    return epoch_base;
}

void rtc_set_epoch_seconds(uint32_t sec) {
    /* Would set the RTC TR/DR registers from epoch seconds.
     * BCD conversion + date computation would go here. */
    (void)sec;
}