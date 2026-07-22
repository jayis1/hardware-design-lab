/*
 * drivers/eddy.h — Eddy-current cover-depth driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_EDDY_H
#define REBARSCOPE_EDDY_H

#include <stdint.h>

void  eddy_init(void);
float eddy_measure_depth_mm(float *out_amp);
float eddy_estimate_diameter_mm(float amp, float depth_mm);
void  eddy_update_calibration(const float *t_p_us, const float *depth_mm, uint8_t n);
void  eddy_powerdown(void);

#endif /* REBARSCOPE_EDDY_H */