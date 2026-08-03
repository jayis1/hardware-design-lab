/*
 * dead_reckon.h — Inertial dead-reckoning with ZUPT and optical-flow fusion
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_DEAD_RECKON_H
#define INKWELL_DRIVERS_DEAD_RECKON_H

#include <stdint.h>

void  dead_reckon_init(uint32_t sample_hz);
void  dead_reckon_update(const float accel_g[3], const float q[4],
                          float dt_s, float a_lin_out[3]);
void  dead_reckon_zupt(void);
void  dead_reckon_fuse_optflow(int16_t dx_counts, int16_t dy_counts, uint8_t squal);
void  dead_reckon_get_delta(float *dx_um, float *dy_um);
void  dead_reckon_clear_delta(void);

#endif