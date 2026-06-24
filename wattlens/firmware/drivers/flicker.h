/*
 * flicker.h — IEC 61000-4-15 flicker (Pst) measurement
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_FLICKER_H
#define WATTLENS_FLICKER_H

#include "board.h"

void flicker_init(void);
void flicker_update(const float *v1, const float *v2, const float *v3, float grid_freq);
float flicker_get_pst(int phase);
float flicker_get_plt(void);

#endif /* WATTLENS_FLICKER_H */