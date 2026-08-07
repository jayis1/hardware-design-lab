/*
 * acoustic.h — Acoustic Bubble Detection Driver Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_ACOUSTIC_H
#define FERMENTIQ_ACOUSTIC_H

#include <stdint.h>

/* API */
int acoustic_init(void);
int acoustic_process(float *bubble_rate, float *centroid, float *rms);

#endif /* FERMENTIQ_ACOUSTIC_H */