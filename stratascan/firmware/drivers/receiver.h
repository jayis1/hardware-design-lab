/*
 * receiver.h — ADS131M08 24-bit ADC Driver (I/Q Capture)
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_RECEIVER_H
#define STRATASCAN_RECEIVER_H

#include <stdint.h>

int  receiver_init(void);
void receiver_read_iq(float *i_out, float *q_out);
void receiver_read_raw(int32_t *i_raw, int32_t *q_raw);
void receiver_reset(void);
void receiver_set_gain(uint8_t channel, uint8_t gain);

/* ADS131M08 configuration constants */
#define ADC_NUM_CHANNELS    8
#define ADC_SAMPLE_RATE_HZ  8000
#define ADC_VREF_V          2.5f
#define ADC_FULLSCALE       8388607.0f  /* 24-bit signed max = 2^23 - 1 */

/* Channel mapping:
 * CH0 = I (in-phase baseband from HMC595 demodulator)
 * CH1 = Q (quadrature baseband from HMC595 demodulator)
 * CH2 = temperature (MCU internal, via external divider)
 * CH3 = battery voltage monitor
 * CH4-7 = spare / auxiliary
 */
#define ADC_CH_I   0
#define ADC_CH_Q   1

#endif /* STRATASCAN_RECEIVER_H */