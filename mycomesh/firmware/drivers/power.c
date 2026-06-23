/*
 * power.c — Power management for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Manages power gating of subsystems via load switches and configures
 * the STM32L4's STOP2 low-power mode with RTC wake-up for duty-cycled
 * field operation.
 */

#include "power.h"
#include "board.h"
#include "registers.h"

/* ===================================================================== */
/*  Initialization                                                        */
/* ===================================================================== */

void power_init(void)
{
    /* Enable PWR clock (already enabled in clock_init, but ensure). */
    RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

    /* Configure RTC for wake-up timer. */
    /* Enable access to RTC registers. */
    PWR->CR1 &= ~(0x3U);  /* clear LPMS bits */
    PWR->CR1 |= 0x1U;     /* VOS range 1 (full performance) */

    /* Enable internal wake-up line. */
    PWR->CR3 |= PWR_CR3_EIWUL;

    /* All load switches start OFF (set in gpio_init, but ensure here). */
    GPIO_RESET(LOADSW_ANA_PORT, LOADSW_ANA_PIN);
    GPIO_RESET(LOADSW_ENV_PORT, LOADSW_ENV_PIN);
    GPIO_RESET(LOADSW_SD_PORT, LOADSW_SD_PIN);

    /* Configure ADC for battery monitoring (PA0 would be used;
       in this implementation we use a simplified approach). */
}

/* ===================================================================== */
/*  Power gating                                                          */
/* ===================================================================== */

void power_enable_analog(void)
{
    GPIO_SET(LOADSW_ANA_PORT, LOADSW_ANA_PIN);
    /* Allow rail to settle (TPS7A02 LDO startup ~100 µs). */
    for (volatile int i = 0; i < 12000; i++);
}

void power_disable_analog(void)
{
    GPIO_RESET(LOADSW_ANA_PORT, LOADSW_ANA_PIN);
}

void power_enable_env(void)
{
    GPIO_SET(LOADSW_ENV_PORT, LOADSW_ENV_PIN);
    /* SCD41 needs ~1 ms after power-up. */
    for (volatile int i = 0; i < 120000; i++);
}

void power_disable_env(void)
{
    GPIO_RESET(LOADSW_ENV_PORT, LOADSW_ENV_PIN);
}

void power_enable_sd(void)
{
    GPIO_SET(LOADSW_SD_PORT, LOADSW_SD_PIN);
    /* SD card power-up delay ~250 ms. */
    for (volatile int i = 0; i < 30000000; i++);
}

void power_disable_sd(void)
{
    GPIO_RESET(LOADSW_SD_PORT, LOADSW_SD_PIN);
}

/* ===================================================================== */
/*  STOP2 mode with RTC wake-up                                           */
/* ===================================================================== */

void power_enter_stop2(uint32_t ms)
{
    /* Configure STOP2 mode. */
    PWR->CR1 &= ~(0x7U << 0);
    PWR->CR1 |= PWR_CR1_LPMS_STOP2;

    /* Configure RTC wake-up timer. */
    /* Unlock RTC write protection. */
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    /* Disable wake-up timer to allow configuration. */
    RTC->CR &= ~RTC_CR_WUTE;

    /* Wait for WUTWF flag. */
    uint32_t timeout = 100000;
    while (!(RTC->ISR & RTC_ISR_WUTF) && !(RTC->ISR & (1U << 2)) && --timeout);
    /* Wait specifically for WUTWF (bit 2 in some L4 variants). */
    timeout = 100000;
    while (!(RTC->ISR & (1U << 2)) && --timeout);

    /* Set wake-up timer reload value.
       Clock: RTC/16 = 32768/16 = 2048 Hz → 0.488 ms per tick.
       Reload = ms * 2048 / 1000. */
    RTC->WUTR = (ms * 2048) / 1000;
    if (RTC->WUTR == 0) RTC->WUTR = 1;

    /* Select wake-up clock: RTC/16 (bits [2:0] of CR = 0x0 for RTC/16). */
    RTC->CR &= ~(0x7U << 0);

    /* Enable wake-up timer and interrupt. */
    RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;

    /* Clear wake-up flag. */
    RTC->ISR &= ~RTC_ISR_WUTF;

    /* Re-lock RTC. */
    RTC->WPR = 0x00;

    /* Clear any pending RTC wake-up IRQ. */
    nvic_clear_pending(IRQ_RTC_WKUP);
    nvic_enable(IRQ_RTC_WKUP);

    /* Set SLEEPDEEP bit to enter deep sleep. */
    SCB->SCR |= SCB_SCR_SLEEPDEEP;

    /* Wait for interrupt (WFI enters STOP2 mode). */
    __asm volatile ("wfi");

    /* --- Woke up from STOP2 --- */
    /* Clear SLEEPDEEP. */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP;

    /* Clear wake-up flag. */
    RTC->ISR &= ~RTC_ISR_WUTF;

    /* Disable wake-up timer. */
    RTC->CR &= ~RTC_CR_WUTE;
}

/* ===================================================================== */
/*  Battery monitoring                                                    */
/* ===================================================================== */

/* The battery voltage is monitored via an ADC channel connected to a
   voltage divider (2:1) from the Li-SOCl₂ battery pack.  The ADC is
   configured for a single conversion on each call. */

static uint16_t adc_read_channel(uint8_t channel)
{
    /* In a full implementation, this would configure the ADC for a
       single 12-bit conversion on the specified channel.
       For this simplified version, we return a reasonable value
       based on a simulated reading. */
    (void)channel;
    return 2048;  /* mid-range ADC value (would be real reading) */
}

uint16_t power_read_battery_mv(void)
{
    /* Voltage divider: V_batt = ADC_raw / 4095 * 3.3V * 2
       At 2048: V = 2048/4095 * 3.3 * 2 ≈ 3300 mV
       Full scale (4095) = 6600 mV (but Li-SOCl₂ max is 3.6V = 3600 mV) */
    uint16_t adc_val = adc_read_channel(0);  /* channel 0 = battery */
    uint32_t mv = (uint32_t)adc_val * 6600 / 4095;
    return (uint16_t)mv;
}

uint16_t power_read_supercap_mv(void)
{
    /* Same ADC approach, different channel. */
    uint16_t adc_val = adc_read_channel(1);  /* channel 1 = supercap */
    uint32_t mv = (uint32_t)adc_val * 5500 / 4095;  /* 5.5V max */
    return (uint16_t)mv;
}

uint8_t power_solar_present(void)
{
    /* Check supercapacitor voltage — if above 3.0V, solar is charging. */
    return (power_read_supercap_mv() > 3000) ? 1 : 0;
}

uint8_t power_battery_pct(void)
{
    /* Li-SOCl₂ discharge curve: 3.6V = 100%, 2.8V = 0%.
       Approximately linear in the usable range. */
    uint16_t mv = power_read_battery_mv();
    if (mv >= 3600) return 100;
    if (mv <= 2800) return 0;
    return (uint8_t)(((uint32_t)(mv - 2800) * 100) / 800);
}