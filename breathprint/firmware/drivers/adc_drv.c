/*
 * adc_drv.c — ADC driver for analog H2 sensor and battery monitoring
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * ADC1 is used in single-ended mode with oversampling for 16-bit effective
 * resolution. Channel 0 = H2 sensor (Membrapor H2-500), Channel 1 = battery
 * voltage divider.
 */

#include "adc_drv.h"
#include "../board.h"

/* ========================================================================
 * ADC Calibration values (populated at init from factory cal)
 * ======================================================================== */

static uint32_t adc_cal_fact = 0;
static uint32_t adc_vref_cal = 0;  /* Factory VREFINT cal (raw) */

/* VREFINT calibration value stored at factory (TS_CAL2 addr for H7) */
#define VREFINT_CAL_ADDR    ((uint16_t *)0x1FF1A830UL)
#define VREFINT_CAL_VREF    3300U   /* 3.3 V reference at calibration */

/* ========================================================================
 * Initialize ADC1
 * ======================================================================== */

void adc_init(adc_reg_t *adc) {
    /* Exit deep power-down */
    adc->CR &= ~ADC_CR_DEEPPWD;
    delay_ms(1);

    /* Enable voltage regulator */
    adc->CR |= ADC_CR_ADVREGEN;
    delay_ms(1);  /* T_ADVREG_STUP max = 20 us */

    /* Configure: 16-bit resolution, right-aligned, continuous mode disabled */
    adc->CFGR = ADC_CFGR_RES_16BIT | ADC_CFGR_ALIGN_RIGHT | ADC_CFGR_OVRMOD;
    adc->CFGR2 = 0;  /* No oversampling for now */

    /* Set sample time for all channels to 810.5 cycles (longest) */
    adc->SMPR1 = 0;
    for (int i = 0; i < 10; i++) {
        adc->SMPR1 |= (ADC_SMPR_810CYC << (i * 3));
    }
    adc->SMPR2 = 0;
    for (int i = 0; i < 10; i++) {
        adc->SMPR2 |= (ADC_SMPR_810CYC << (i * 3));
    }

    /* Enable ADC */
    adc->ISR = 0xFFFFFFFF;  /* Clear all flags */
    adc->CR |= ADC_CR_ADEN;
    while (!(adc->ISR & ADC_ISR_ADRDY));
    adc->ISR = ADC_ISR_ADRDY;

    /* Read VREFINT calibration */
    adc_vref_cal = *VREFINT_CAL_ADDR;
}

/* ========================================================================
 * Read single channel (blocking)
 * ======================================================================== */

uint16_t adc_read_channel(adc_reg_t *adc, uint8_t channel) {
    /* Configure sequence: 1 conversion, channel = requested */
    adc->SQR1 = (channel << 6) | (0U << 0);  /* L=0 means 1 conversion */

    /* Clear EOC flag */
    adc->ISR = ADC_ISR_EOC;

    /* Start conversion */
    adc->CR |= ADC_CR_ADSTART;

    /* Wait for end of conversion */
    uint32_t start = millis();
    while (!(adc->ISR & ADC_ISR_EOC)) {
        if (millis() - start > 100) return 0;  /* Timeout */
    }

    /* Read result */
    uint16_t value = (uint16_t)adc->DR;

    /* Stop conversion */
    adc->CR |= ADC_CR_ADSTP;
    while (adc->CR & ADC_CR_ADSTP);

    return value;
}

/* ========================================================================
 * Read with oversampling (software)
 * ======================================================================== */

uint16_t adc_read_oversampled(adc_reg_t *adc, uint8_t channel,
                              uint8_t num_samples) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < num_samples; i++) {
        sum += adc_read_channel(adc, channel);
    }
    return (uint16_t)(sum / num_samples);
}

/* ========================================================================
 * Convert ADC value to voltage (using VREFINT for accuracy)
 * ======================================================================== */

float adc_to_voltage(uint16_t raw) {
    /* Read VREFINT to determine actual VREF */
    uint16_t vref_raw = adc_read_channel(ADC1, 19);  /* VREFINT channel */

    /* Calculate actual VDD using factory calibration */
    float vdd = (float)VREFINT_CAL_VREF * (float)adc_vref_cal / (float)vref_raw;

    /* Convert raw to voltage */
    return vdd * (float)raw / 65535.0f;
}

/* ========================================================================
 * Read H2 sensor (Membrapor H2-500)
 *   Analog output: 0.4 - 2.0 V corresponds to 0 - 500 ppm H2
 *   Transfer function: ppm = (V_out - 0.4) / (2.0 - 0.4) * 500
 * ======================================================================== */

float adc_read_h2(void) {
    uint16_t raw = adc_read_oversampled(ADC1, H2_ADC_CHANNEL, 16);
    float voltage = adc_to_voltage(raw);

    /* Transfer function (with clamping) */
    if (voltage < 0.4f) return 0.0f;

    float ppm = (voltage - 0.4f) / 1.6f * 500.0f;
    if (ppm > 500.0f) ppm = 500.0f;

    /* Temperature compensation (approximate, from BMP390 temp) */
    /* TODO: Apply proper temp compensation from calibration data */

    return ppm;
}

/* ========================================================================
 * Read battery voltage
 *   Voltage divider: VBAT × (10k / (10k + 20k)) = VBAT / 3 → ADC
 *   Full-scale Li-Po: 4.2V → 1.4V at ADC
 * ======================================================================== */

uint8_t adc_read_battery_pct(void) {
    uint16_t raw = adc_read_oversampled(ADC1, BAT_ADC_CHANNEL, 8);
    float voltage = adc_to_voltage(raw) * 3.0f;  /* Undo divider */

    /* Li-Po discharge curve (approximate) */
    /* 4.2V = 100%, 3.0V = 0% */
    float pct = (voltage - 3.0f) / 1.2f * 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;

    return (uint8_t)pct;
}

/* ========================================================================
 * Read battery voltage directly (for logging)
 * ======================================================================== */

float adc_read_battery_voltage(void) {
    uint16_t raw = adc_read_oversampled(ADC1, BAT_ADC_CHANNEL, 8);
    return adc_to_voltage(raw) * 3.0f;
}

/* ========================================================================
 * Continuous mode with DMA (for sensor sampling)
 * ======================================================================== */

static volatile uint16_t adc_dma_buffer[4];
static volatile uint8_t adc_dma_done = 0;

void adc_start_continuous(adc_reg_t *adc, uint8_t *channels,
                          uint8_t num_channels) {
    /* Configure sequence */
    uint32_t sqr = 0;
    sqr |= ((uint32_t)(num_channels - 1) << 0);  /* L = num-1 */
    for (uint8_t i = 0; i < num_channels && i < 4; i++) {
        sqr |= ((uint32_t)channels[i] << (6 + i * 6));
    }
    adc->SQR1 = sqr;

    /* Enable DMA mode */
    adc->CFGR |= ADC_CFGR_DMAEN | ADC_CFGR_DMACFG;  /* Circular DMA */
    adc->CFGR |= ADC_CFGR_CONT;  /* Continuous mode */

    /* Configure DMA stream */
    DMA1_STR2->CR = DMA_CR_MINC | DMA_CR_PSIZE_16BIT | DMA_CR_MSIZE_16BIT |
                    DMA_CR_DIR_P2M | DMA_CR_CIRC | DMA_CR_TCIE;
    DMA1_STR2->NDTR = num_channels;
    DMA1_STR2->PAR = (uint32_t)&adc->DR;
    DMA1_STR2->M0AR = (uint32_t)adc_dma_buffer;
    DMA1_STR2->CR |= DMA_CR_EN;

    /* Start conversion */
    adc->CR |= ADC_CR_ADSTART;
}

void adc_stop_continuous(adc_reg_t *adc) {
    adc->CR |= ADC_CR_ADSTP;
    while (adc->CR & ADC_CR_ADSTP);

    DMA1_STR2->CR &= ~DMA_CR_EN;
    adc->CFGR &= ~(ADC_CFGR_DMAEN | ADC_CFGR_DMACFG | ADC_CFGR_CONT);
}

uint16_t adc_get_dma_value(uint8_t index) {
    if (index < 4) return adc_dma_buffer[index];
    return 0;
}