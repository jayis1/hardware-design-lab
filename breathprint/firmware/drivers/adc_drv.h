/*
 * adc_drv.h — ADC driver header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef ADC_DRV_H
#define ADC_DRV_H

#include <stdint.h>
#include "../registers.h"

void adc_init(adc_reg_t *adc);
uint16_t adc_read_channel(adc_reg_t *adc, uint8_t channel);
uint16_t adc_read_oversampled(adc_reg_t *adc, uint8_t channel,
                              uint8_t num_samples);
float adc_to_voltage(uint16_t raw);
float adc_read_h2(void);
uint8_t adc_read_battery_pct(void);
float adc_read_battery_voltage(void);
void adc_start_continuous(adc_reg_t *adc, uint8_t *channels,
                          uint8_t num_channels);
void adc_stop_continuous(adc_reg_t *adc);
uint16_t adc_get_dma_value(uint8_t index);

#endif /* ADC_DRV_H */