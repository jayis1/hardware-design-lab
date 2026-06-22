/*
 * spectrometer.h — AS7343 11-channel spectral sensor driver
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_SPECTROMETER_H
#define LUMICAST_SPECTROMETER_H

#include <stdint.h>
#include "board.h"

/* Raw channel counts after I2C read (0..65535). */
typedef struct {
    uint16_t f1_415;
    uint16_t f2_445;
    uint16_t f3_480;
    uint16_t f4_515;
    uint16_t f5_555;
    uint16_t f6_590;
    uint16_t f7_630;
    uint16_t f8_680;
    uint16_t nir_910;
    uint16_t clear;
    uint16_t flicker;
} as7343_raw_t;

/* Normalized spectral radiance estimate (counts corrected for gain,     */
/* integration time, channel responsivity and dark offset).              */
typedef struct {
    float irradiance_uw_cm2[AS7343_NUM_CHANNELS];
    float total_irradiance;
    uint16_t raw[AS7343_NUM_CHANNELS];
    uint32_t timestamp_ms;
    uint8_t gain;        /* 0.5X..512X, encoded */
    uint16_t atime;      /* integration time step count */
} spectral_sample_t;

/* API */
int  as7343_init(void);
int  as7343_set_gain(uint8_t gain_step);
int  as7343_set_atime(uint8_t atime);
int  as7343_enable(bool on);
int  as7343_read_raw(as7343_raw_t *out);
int  as7343_sample(spectral_sample_t *out);
uint8_t as7343_get_gain(void);
uint16_t as7343_get_atime(void);
float as7343_gain_multiplier(uint8_t gain_step);

#endif /* LUMICAST_SPECTROMETER_H */