/*
 * adc.h — ADS1255 24-bit ADC driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_ADC_H
#define DRIVERS_ADC_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Initialize ADS1255: SPI setup, reset, configure PGA gain + data rate */
bool adc_init(void);

/* Acquire full 128-element frame by multiplexing the photodiode array.
 * Each element is read as a 24-bit signed sample.
 * integ_ms controls the integration/settling time per element.
 * Returns true on success. */
bool adc_acquire_frame(int32_t *frame, uint32_t integ_ms);

/* Read a single 24-bit sample from the current MUX channel */
int32_t adc_read_single(void);

/* Set PGA gain (1, 2, 4, 8, 16, 32, 64) */
void adc_set_gain(uint8_t gain);

/* Set data rate */
void adc_set_drate(uint8_t drate_reg);

/* Perform self-calibration */
bool adc_self_cal(void);

/* Software reset */
void adc_reset(void);

/* Read register */
uint8_t adc_read_reg(uint8_t reg);

/* Write register */
void adc_write_reg(uint8_t reg, uint8_t val);

#endif /* DRIVERS_ADC_H */