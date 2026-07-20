/*
 * ad5940.h — AD5940 Bioimpedance AFE Driver (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_AD5940_H
#define ROOTTRACE_AD5940_H

#include <stdint.h>
#include "board.h"

/* Measurement result (complex impedance) */
typedef struct {
    int32_t real;   /* DFT real part (scaled) */
    int32_t imag;   /* DFT imaginary part (scaled) */
    uint32_t freq;  /* Excitation frequency (Hz) */
    uint32_t mag;   /* Magnitude (computed) */
    int32_t phase_mdeg; /* Phase in millidegrees */
} ad5940_meas_t;

/* AD5940 configuration */
typedef struct {
    uint32_t freq_hz;        /* Excitation frequency */
    uint32_t current_ua;     /* Current amplitude (µA) */
    uint8_t  pga_gain;        /* PGA gain (0=1, 1=1.5, 2=2, ..., 7=9) */
    uint8_t  dft_len_log2;    /* DFT length: 0=16, 1=32, ..., 6=1024 */
    uint8_t  adc_rate_div;    /* ADC rate divider */
} ad5940_config_t;

/* Function prototypes */
void ad5940_init(void);
void ad5940_reset(void);
void ad5940_configure(const ad5940_config_t *cfg);
void ad5940_reg_write(uint16_t reg, uint32_t val);
uint32_t ad5940_reg_read(uint16_t reg);
int ad5940_measure(ad5940_meas_t *result);
int ad5940_measure_sweep(const uint32_t *freqs, int n_freq,
                          ad5940_meas_t *results);
int ad5940_self_test(void);
void ad5940_sleep(void);
void ad5940_wakeup(void);
void ad5940_start_dft(void);
void ad5940_wait_dft_done(uint32_t timeout_ms);

#endif /* ROOTTRACE_AD5940_H */