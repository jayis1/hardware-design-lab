/*
 * drivers/adc.c — ADC driver for STM32L432KC
 *
 * Reads the multiplexed Hall-effect signal, the 1:3 battery divider
 * (channel 9 = PB0) and the solar-panel divider (channel 15 = PB1 alt).
 * The ADC is powered down between conversions to save energy.
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "../board.h"
#include "../registers.h"
#include "adc.h"

/* ---- Private helpers ---------------------------------------------- */

static void adc_enable_clock(void)
{
    RCC_AHB2ENR |= RCC_AHB2ENR_ADCEN;
    /* small dummy read to ensure clock is on */
    (void)RCC_AHB2ENR;
}

static void adc_calibrate(void)
{
    /* ADC must be disabled to calibrate */
    ADC_CR &= ~ADC_CR_ADEN;
    /* single-ended calibration */
    ADC_CR |= ADC_CR_ADCAL_BIT;
    while (ADC_CR & ADC_CR_ADCAL_BIT) { /* wait */ }
}

/* ---- Public API --------------------------------------------------- */

void adc_init(void)
{
    adc_enable_clock();

    /* Configure PA0 (HALL), PB0 (VBAT), PB1 (SOLAR) as analog */
    GPIO_MODER(GPIOA_BASE) |= (GPIO_MODE_ANALOG << (0u * 2u));  /* PA0 */
    GPIO_MODER(GPIOB_BASE) |= (GPIO_MODE_ANALOG << (0u * 2u));  /* PB0 */
    GPIO_MODER(GPIOB_BASE) |= (GPIO_MODE_ANALOG << (1u * 2u));  /* PB1 */

    /* Enable ADC voltage regulator and wait for it to stabilise */
    ADC_CR |= ADC_CR_ADVREG_EN;
    /* tADVREG_STUP ~ 20 us; conservative loop */
    for (volatile uint32_t i = 0; i < 2000u; i++) { __asm__("nop"); }

    adc_calibrate();

    /* Enable ADC */
    ADC_CR |= ADC_CR_ADEN;
    while ((ADC_ISR & ADC_ISR_ADRDY) == 0u) { /* wait for ready */ }

    /* Configure: 12-bit, single conversion, software trigger, right align */
    ADC_CFGR = 0x0u;  /* defaults: 12-bit, right-aligned, SW start */
}

/*
 * Read a single ADC channel (blocking, ~ 20 us).
 * Channels: 5 = HALL (PA0), 9 = VBAT (PB0), 15 = SOLAR (PB1)
 */
uint16_t adc_read(uint8_t channel)
{
    /* Select channel in SQR1[4:0] (L=0 -> 1 conversion) */
    ADC_SQR1 = (uint32_t)channel & 0x1Fu;

    /* Sample time: 24.5 cycles (SMP = 3 = 24.5 ADC clk cycles) */
    if (channel < 10u) {
        ADC_SMPR1 = (3u << (channel * 3u));
    } else {
        ADC_SMPR2 = (3u << ((channel - 10u) * 3u));
    }

    /* Start conversion */
    ADC_CR |= ADC_CR_ADSTART;
    /* Wait for conversion to complete (EOC via polling DR) */
    while ((ADC_ISR & (1u << 2u)) == 0u) { /* EOC flag */ }

    uint16_t val = (uint16_t)(ADC_DR & 0x0FFFu);

    /* Clear EOC by reading DR (already done) */
    return val;
}

/*
 * Battery voltage in mV. External divider 1:3 (Rtop = 2 M, Rbot = 1 M).
 * Vref = 3.0 V (VDDA). 12-bit ADC -> N/4096 * 3000 mV at divider node,
 * then x3 for actual battery voltage. Use 10x oversample for stability.
 */
uint16_t adc_read_vbat_mv(void)
{
    uint32_t acc = 0u;
    for (uint8_t i = 0; i < 10u; i++) {
        acc += adc_read(VBAT_ADC_CH);
    }
    acc /= 10u;
    /* node_mV = acc * 3000 / 4096 ; batt = node_mV * 3 */
    uint32_t node_mv = (acc * 3000u) / 4096u;
    return (uint16_t)(node_mv * 3u);
}

/*
 * Solar panel voltage in mV (same 1:3 divider topology as VBAT).
 */
uint16_t adc_read_solar_mv(void)
{
    uint32_t acc = 0u;
    for (uint8_t i = 0; i < 10u; i++) {
        acc += adc_read(SOLAR_ADC_CH);
    }
    acc /= 10u;
    uint32_t node_mv = (acc * 3000u) / 4096u;
    return (uint16_t)(node_mv * 3u);
}

/*
 * Power down ADC for sleep. Will be re-enabled on next read.
 */
void adc_enter_lowpower(void)
{
    ADC_CR &= ~ADC_CR_ADEN;       /* disable ADC */
    ADC_CR &= ~ADC_CR_ADVREG_EN;  /* disable voltage regulator */
    RCC_AHB2ENR &= ~RCC_AHB2ENR_ADCEN;
}