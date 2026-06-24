/*
 * power_calc.h — Power calculation module
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_POWER_CALC_H
#define WATTLENS_POWER_CALC_H

#include "board.h"

void power_calc_init(void);
void power_calc_compute(const float *v[3], const float *i[4],
                        power_metrics_t *m, const cal_data_t *cal);
float power_calc_rms(const float *x, int n);
float power_calc_frequency(const float *v, int n, float fs);

#endif /* WATTLENS_POWER_CALC_H */