/*
 * mux.h — 16-Electrode Multiplexer Driver (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_MUX_H
#define ROOTTRACE_MUX_H

#include <stdint.h>

/* Electrode selector: selects which electrode is connected to which
 * AD5940 channel (excitation+, excitation-, measurement+, measurement-) */
typedef struct {
    uint8_t exc_pos;   /* 0..15, electrode for positive excitation current */
    uint8_t exc_neg;   /* 0..15, electrode for return current */
    uint8_t meas_pos;  /* 0..15, electrode for positive measurement input */
    uint8_t meas_neg;  /* 0..15, electrode for negative measurement input */
} mux_config_t;

void mux_init(void);
void mux_configure(const mux_config_t *cfg);
void mux_disconnect_all(void);
int  mux_check_contact(uint8_t electrode);  /* returns 1 if contact OK */
void mux_test_pattern(void);

#endif /* ROOTTRACE_MUX_H */