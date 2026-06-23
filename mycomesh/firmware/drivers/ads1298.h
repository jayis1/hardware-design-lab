/*
 * ads1298.h — ADS1298 dual biomedical ADC driver header
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_ADS1298_H
#define MYCOMESH_ADS1298_H

#include "board.h"
#include <stdint.h>

/* Initialize both ADS1298 devices (16 channels total). */
int ads1298_init(void);

/* Start continuous acquisition on both devices. */
void ads1298_start(void);

/* Stop acquisition and enter standby. */
void ads1298_stop(void);

/* Power down both ADS1298 devices (for sleep mode). */
void ads1298_power_down(void);

/* Wake from power-down. */
void ads1298_wake(void);

/* Read a block of n_samples from all 16 channels.
 * Returns 0 on success, -1 on timeout/error.
 * Output buffer: buf[16][n_samples] of 16-bit signed values (MSB-aligned). */
int ads1298_read_block(int16_t buf[ADS1298_TOTAL_CHANS][DSP_BLOCK_SIZE],
                       uint16_t n_samples);

/* Read a single sample from all 16 channels (non-blocking poll). */
int ads1298_read_sample(int16_t samples[ADS1298_TOTAL_CHANS]);

/* Check lead-off status for all channels.
 * Sets lead_off[ch] = 1 if electrode is disconnected. */
void ads1298_check_lead_off(uint8_t lead_off[ADS1298_TOTAL_CHANS]);

/* Set PGA gain for all channels (1, 2, 3, 4, 6, 8, 12). */
void ads1298_set_gain(uint8_t gain);

/* Configure sample rate (125, 250, 500, 1000, 2000, 4000, 8000, 16000 SPS). */
void ads1298_set_sample_rate(uint16_t sps);

/* Perform offset calibration. */
void ads1298_calibrate_offset(void);

/* Write a register on a specific ADS1298 device (dev 0 or 1). */
void ads1298_write_reg(uint8_t dev, uint8_t reg, uint8_t value);

/* Read a register from a specific ADS1298 device. */
uint8_t ads1298_read_reg(uint8_t dev, uint8_t reg);

#endif /* MYCOMESH_ADS1298_H */