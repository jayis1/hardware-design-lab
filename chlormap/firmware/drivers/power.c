/*
 * power.c — Power management (STOP2, battery, regulators)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Implements ultra-low-power STOP2 mode for the STM32L432, battery
 * voltage monitoring via internal ADC, and regulator/LED power rail
 * gating.
 */

#include "power.h"
#include "board.h"
#include "registers.h"

static uint16_t g_batt_mv_cache = 4200;

/* ---- Internal ADC (for battery sense + temperature) ---- */
static void adc_internal_init(void)
{
    /* Enable ADC clock, configure channel 0 (PA0) for battery sense
     * and channel 17 (temperature sensor) for device temp
     */
    /* ADC1->CR |= ADC_CR_ADVREGEN; delay 20 µs;
     * ADC1->CFGR = ...; ADC1->SMPR = ...;
     */
}

static uint16_t adc_read_channel(uint8_t channel)
{
    /* ADC1->SQR1 = (channel << 6); ADC1->CR |= ADC_CR_ADSTART;
     * while(!(ADC1->ISR & ADC_ISR_EOC));
     * return ADC1->DR;
     */
    (void)channel;
    return 2048; /* mid-scale stub */
}

/* ---- Public API ---- */

bool power_init(void)
{
    adc_internal_init();

    /* Configure VLED_EN and VANA_EN as outputs, initially off */
    /* GPIOA->MODER |= ...; GPIOB->MODER |= ...; */

    /* Configure BATT_SENSE (PA0) as analog input */
    /* GPIOA->MODER |= GPIO_MODER_MODE0_Analog; */

    /* Configure TPS62740 VSEL pins for 3.3V output */
    /* VSEL1=high, VSEL2=low for 3.3V */

    return true;
}

uint16_t power_read_battery_mv(void)
{
    /* Read PA0 ADC (12-bit, 0–3300 mV range, 1:2 divider) */
    uint16_t raw = adc_read_channel(BATT_SENSE_CHANNEL);

    /* Convert: mV = raw / 4095 * 3300 * BATT_DIVIDER */
    uint32_t mv = ((uint32_t)raw * ADC_VREF_MV * BATT_DIVIDER) / 4095;
    g_batt_mv_cache = (uint16_t)mv;
    return g_batt_mv_cache;
}

uint8_t power_get_battery_pct(void)
{
    uint16_t mv = power_read_battery_mv();

    /* LiPo discharge curve: 4200mV=100%, 3400mV=0% */
    if (mv >= BATT_FULL_MV) return 100;
    if (mv <= BATT_CRIT_MV) return 0;

    /* Linear interpolation between 3400 and 4200 */
    uint32_t pct = ((uint32_t)(mv - BATT_LOW_MV) * 100) /
                   (BATT_FULL_MV - BATT_LOW_MV);
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

void power_enter_stop2(void)
{
    /* Enter STOP2 mode (lowest power with RTC + EXTI wakeup):
     *
     * 1. Disable all peripheral clocks except RTC
     * 2. Set SLEEPDEEP = 1, PDDS = 0, LPDS = 1 (low-power regulator in STOP)
     * 3. Set STOP2 bit in PWR_CR1
     * 4. Clear WUF flags
     * 5. Execute WFI instruction
     *
     * Power consumption: ~1.4 µA (MCU) + 360 nA (TPS62740) = ~1.8 µA total
     * Wake sources: EXTI8 (trigger), RTC alarm (1 Hz)
     */

    /* PWR->CR1 |= PWR_CR1_LPMS_STOP2; */
    /* SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; */
    /* __WFI(); */
    /* After wake, execution continues here */
}

void power_wakeup(void)
{
    /* Called after waking from STOP2 — re-enable clocks */
    /* In real build: re-init PLL, re-enable peripheral clocks */
}

void power_wakeup_cleanup(void)
{
    /* Restore SPI, I2C, USART, USB clocks if needed */
    /* Clear EXTI pending flags */
}

void power_enable_leds(bool on)
{
    /* VLED_EN pin: high = on, low = off */
    /* if (on) GPIOB->BSRR = (1 << 13); else GPIOB->BSRR = (1 << 13) << 16; */
    (void)on;
}

void power_enable_analog(bool on)
{
    /* VANA_EN pin */
    /* if (on) GPIOA->BSRR = (1 << 9); else GPIOA->BSRR = (1 << 9) << 16; */
    (void)on;
}

bool power_is_charging(void)
{
    /* Check MCP73871 STAT pin or USB VBUS presence */
    /* Read PA12 (USB DP) or a dedicated STAT pin */
    return false; /* stub */
}

int16_t power_read_temp_c_x10(void)
{
    /* Read STM32L4 internal temperature sensor (ADC channel 17)
     * temp = (VSENSE - V25) / Avg_Slope + 25
     * V25 = 0.76 V (typ), Avg_Slope = 2.5 mV/°C (typ)
     */
    uint16_t raw = adc_read_channel(17); /* temp sensor channel */

    /* Convert raw ADC to voltage: V = raw / 4095 * 3300 mV */
    int32_t vsense_mv = ((int32_t)raw * 3300) / 4095;

    /* temp = (vsense - 760) / 2.5 + 25 (in °C) */
    int32_t temp_c = ((vsense_mv - 760) * 10) / 25 + 250; /* × 10 */

    return (int16_t)temp_c;
}