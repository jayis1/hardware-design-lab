/*
 * ads1292.h — ADS1292 24-bit biopotential ADC driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_ADS1292_H
#define FLORAPULSE_ADS1292_H

#include <stdint.h>

/* Initialize the ADS1292 via SPI1.
 * Configures:
 *  - HR mode, 250 SPS (high resolution)
 *  - PGA gain 12 (for plant AP signals ~µV range)
 *  - Internal reference enabled
 *  - Both channels in normal input mode
 *  - Internal test signal disabled
 */
void ads1292_init(void);

/* Start continuous conversions. DRDY will assert at 250 SPS.
 * Connects EXTI1 interrupt → dma_read.
 */
void ads1292_start(void);

/* Stop conversions */
void ads1292_stop(void);

/* Read latest 24-bit sample for channel (1 or 2).
 * Returns value in ADC counts (signed).
 */
int32_t ads1292_read_channel(uint8_t ch);

/* Read latest samples for both channels at once.
 * ch1, ch2 receive 24-bit signed values.
 */
void ads1292_read_both(int32_t *ch1, int32_t *ch2);

/* Convert ADC counts to microvolts using current gain setting */
float ads1292_counts_to_uv(int32_t counts);

/* Trigger internal test signal (square wave) for validation */
void ads1292_test_signal_enable(void);
void ads1292_test_signal_disable(void);

/* Detect action potential events in the ring buffer.
 * Returns count of events detected in the last analysis window.
 * Threshold in µV.
 */
uint16_t ads1292_detect_events(float threshold_uv, uint16_t window_samples);

/* Get RMS amplitude over last N samples */
float ads1292_get_rms(uint16_t n_samples);

#endif /* FLORAPULSE_ADS1292_H */