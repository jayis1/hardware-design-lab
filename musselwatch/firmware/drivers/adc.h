/*
 * drivers/adc.h — ADC driver for STM32L432KC (gape, battery, solar)
 *
 * Author:  jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#ifndef MUSSEL_ADC_H
#define MUSSEL_ADC_H

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read(uint8_t channel);
uint16_t adc_read_vbat_mv(void);
uint16_t adc_read_solar_mv(void);
void     adc_enter_lowpower(void);

#endif