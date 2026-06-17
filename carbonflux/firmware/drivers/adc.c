/**
 * @file    adc.c
 * @brief   ADC driver for PAR sensor, battery voltage, and spare channels.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include "adc.h"
#include "../board.h"
#include "../registers.h"

#define ADC_OVERSAMPLING        64
#define ADC_RESOLUTION          4095

static float vref_mv = 3300.0f;  /* Internal VREF (calibrated) */

void adc_init(void) {
    /* Already initialized in main.c adc_init() */
    /* Read VREFINT calibration from system memory */
}

static uint32_t adc_read_channel(uint8_t channel) {
    /* Set sequence to single channel */
    ADC1->SQR1 = (1 << 0);     /* Length = 1 */
    ADC1->SQR3 = channel;       /* SQ1 = channel */

    /* Start conversion */
    ADC1->CR |= (1U << 2);      /* ADSTART */

    /* Wait for EOC */
    while (!(ADC1->ISR & (1U << 2)));

    uint32_t raw = ADC1->DR & 0xFFFF;
    ADC1->ISR = (1U << 2);      /* Clear EOC */

    return raw;
}

float adc_read_par(void) {
    uint32_t raw = adc_read_channel(0);  /* PC0 = ADC1_IN0 */
    /* PAR sensor (Apogee SQ-420): 0–2.5V → 0–3000 µmol·m⁻²·s⁻¹ */
    float mv = (float)raw * vref_mv / ADC_RESOLUTION;
    return mv * 3000.0f / 2500.0f;  /* µmol·m⁻²·s⁻¹ */
}

float adc_read_vbat(void) {
    uint32_t raw = adc_read_channel(1);  /* PC1 = ADC1_IN1 */
    /* Voltage divider: R1=100k, R2=10k → multiply by 11 */
    float mv_at_pin = (float)raw * vref_mv / ADC_RESOLUTION;
    return mv_at_pin * VBAT_DIVIDER_RATIO / 1000.0f;  /* Volts */
}

uint16_t adc_read_spare(void) {
    return (uint16_t)adc_read_channel(2);
}