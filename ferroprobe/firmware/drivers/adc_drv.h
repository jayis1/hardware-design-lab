/*
 * adc_drv.h — ADS1256 24-bit ADC driver interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_ADC_DRV_H
#define FERROPROBE_ADC_DRV_H

#include <stdint.h>

/* Initialize the ADS1256 24-bit ADC via SPI1.
 * Configures PGA gain = 1x, data rate = 30 kSPS, and sets up
 * DMA for continuous data transfer.
 */
void adc_init(void);

/* Read a single conversion from the specified channel (0-7).
 * Performs a multiplexer change + sync + wait + read sequence.
 * Returns the 24-bit signed value.
 */
int32_t adc_read_channel(uint8_t channel);

/* Start continuous conversion mode with DMA.
 * The ADS1256 DRDY pin triggers an EXTI interrupt, which
 * initiates an SPI DMA read of the 24-bit data register.
 */
void adc_start_continuous(void);

/* Stop continuous mode */
void adc_stop_continuous(void);

/* Set the PGA gain (1, 2, 4, 8, 16, 32) */
void adc_set_pga(uint8_t gain);

/* Set the data rate */
void adc_set_drate(uint8_t drate_code);

/* Perform a self-calibration (offset + gain) */
void adc_self_cal(void);

/* Get the latest DMA-acquired sample for a channel */
int32_t adc_get_latest(uint8_t channel);

/* Number of channels being multiplexed (4: X, Y, Z, temp) */
#define ADC_N_CHANNELS  4

#endif /* FERROPROBE_ADC_DRV_H */