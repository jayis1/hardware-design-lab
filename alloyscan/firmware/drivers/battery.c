/*
 * battery.c — Battery monitoring driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Monitors the 18650 Li-ion battery voltage via ADC1.
 * Uses a 2:1 resistor divider on PA1 (ADC2_IN1).
 */

#include "battery.h"
#include "registers.h"
#include "board.h"
#include <stdbool.h>

void battery_init(void)
{
    /* Enable ADC12 clock */
    RCC_AHB2ENR |= RCC_AHB2ENR_ADC12;

    /* Configure PA1 as analog input */
    GPIOA->MODER &= ~(3UL << (BATTERY_ADC_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_ANALOG << (BATTERY_ADC_PIN * 2));

    /* Enable ADC1 */
    ADC1_CR |= ADC_CR_ADEN;
    while (!(ADC1_ISR & ADC_ISR_ADRDY))
        ;
}

uint16_t battery_read_mv(void)
{
    /* Configure channel 1 (PA1 = ADC2_IN1), single conversion */
    /* For simplicity, use ADC1 channel 1 */
    ADC1_SQR1 = (1UL << 0);  /* 1 conversion, channel 1 (simplified) */

    /* Start conversion */
    ADC1_CR |= ADC_CR_ADSTART;

    /* Wait for conversion complete */
    uint32_t timeout = 10000;
    while (!(ADC1_ISR & (1UL << 2)) && timeout-- > 0)  /* EOC flag */
        ;

    /* Read result */
    uint16_t raw = (uint16_t)ADC1_DR;

    /* Convert to millivolts:
     * ADC is 12-bit (0-4095), Vref = 3.3V
     * Voltage at pin = raw * 3300 / 4095
     * Battery voltage = pin voltage * BATTERY_DIVIDER (2.0) */
    uint32_t pin_mv = ((uint32_t)raw * 3300UL) / 4095UL;
    uint32_t batt_mv = (uint32_t)(pin_mv * BATTERY_DIVIDER);

    return (uint16_t)batt_mv;
}

uint8_t battery_percent(void)
{
    uint16_t mv = battery_read_mv();

    /* Li-ion discharge curve approximation */
    if (mv >= 4150) return 100;
    if (mv >= 4000) return 90 + ((mv - 4000) * 10) / 150;
    if (mv >= 3700) return 60 + ((mv - 3700) * 30) / 300;
    if (mv >= 3500) return 20 + ((mv - 3500) * 40) / 200;
    if (mv >= 3400) return 5 + ((mv - 3400) * 15) / 100;
    if (mv >= 3100) return ((mv - 3100) * 5) / 300;
    return 0;
}

bool battery_is_low(void)
{
    return battery_read_mv() < BATTERY_LOW_MV;
}

bool battery_is_critical(void)
{
    return battery_read_mv() < BATTERY_CRIT_MV;
}