/*
 * adc_drv.h — ADS8866 16-bit SPI ADC driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_ADC_DRV_H
#define ALLOYSCAN_ADC_DRV_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize SPI1 and GPIO for ADS8866 ADC */
void adc_drv_init(void);

/* Trigger a conversion and read one sample (blocking) */
uint16_t adc_drv_read_single(void);

/* Read a block of samples using DMA (non-blocking start)
 * buffer: destination buffer (must be at least count entries)
 * count: number of samples to read
 * Returns true if acquisition started successfully */
bool adc_drv_read_block(uint16_t *buffer, uint32_t count);

/* Check if block read is complete */
bool adc_drv_block_complete(void);

/* Wait for block read to complete (blocking, with timeout ms) */
bool adc_drv_wait_block(uint32_t timeout_ms);

/* Convert raw ADC code to signed voltage (centered at 0)
 * raw: 16-bit ADC code (0-65535)
 * vref: reference voltage in millivolts (typically 3300)
 * Returns signed value in ADC counts (centered around midpoint) */
int16_t adc_drv_to_signed(uint16_t raw, uint16_t vref_mv);

#endif /* ALLOYSCAN_ADC_DRV_H */