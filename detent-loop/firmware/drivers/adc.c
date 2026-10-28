/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "registers.h"
#define ADC_EOC (1u << 1)
#define ADC_OVR (1u << 5)
#define ADC_START (1u << 30)
static const unsigned channels[4] = {0,1,4,8};
bool adc_init(void)
{
    RCC_APB2ENR |= 1u << 8;
    (void)RCC_APB2ENR;
    ADC_CR2 = 0;
    ADC_CR1 = 0;
    /* PCLK2 / 2 = 8MHz. Single conversion, 12-bit, software trigger. */
    ADC_CCR = 0;
    ADC_SMPR1 = 0;
    ADC_SMPR2 = (7u << 0) | (7u << 3) | (7u << 12) | (7u << 24);
    ADC_SQR1 = 0;
    ADC_SQR2 = 0;
    ADC_SQR3 = 0;
    ADC_SR = 0;
    ADC_CR2 = 1u;
    /* Conservative stabilization delay, not an ADC timing calibration. */
    for (volatile unsigned i = 0; i < 16000u; ++i) { }
    return (ADC_CR2 & 1u) != 0;
}
static bool convert(unsigned channel,uint16_t *result)
{
    ADC_SQR3 = channel;
    ADC_SR = 0;
    ADC_CR2 |= ADC_START;
    for (unsigned budget = 0; budget < 100000u; ++budget) {
        uint32_t status = ADC_SR;
        if (status & ADC_OVR)
            return false;
        if (status & ADC_EOC) {
            *result = (uint16_t)(ADC_DR & 4095u);
            return true;
        }
    }
    /* Stop a stuck converter. Do not manufacture a valid measurement. */
    ADC_CR2 = 0;
    return false;
}
bool adc_read(uint16_t output[4])
{
    if (!output || !(ADC_CR2 & 1u))
        return false;
    uint16_t staged[4];
    for (unsigned i = 0; i < 4; ++i) {
        uint16_t sample;
        /* Discard the first sample after mux selection; then average eight. */
        if (!convert(channels[i], &sample))
            return false;
        uint32_t sum = 0;
        for (unsigned j = 0; j < 8; ++j) {
            if (!convert(channels[i], &sample))
                return false;
            sum += sample;
        }
        staged[i] = (uint16_t)((sum + 4u) / 8u);
    }
    for (unsigned i = 0; i < 4; ++i)
        output[i] = staged[i];
    return true;
}
