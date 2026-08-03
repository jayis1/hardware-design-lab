/*
 * pressure.h — HX711 nib pressure driver and pen-lift FSM
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_PRESSURE_H
#define INKWELL_DRIVERS_PRESSURE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t force_mN;  /* calibrated force in millinewtons */
    uint32_t ts_ms;     /* timestamp */
} pressure_sample_t;

void     pressure_init(void (*on_sample)(const pressure_sample_t *));
void     pressure_update(uint16_t force_mN, uint32_t ts_ms);
bool     pressure_is_pen_down(void);
uint16_t pressure_get_force_mN(void);
void     pressure_set_calibration(int32_t zero_offset, float scale_n_per_lsb);
void     pressure_get_calibration(int32_t *zero_offset, float *scale_n_per_lsb);

#endif