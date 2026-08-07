/*
 * ph_isfet.h — ISFET pH Sensor Driver Header (LMP91200 front-end)
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_PH_ISFET_H
#define FERMENTIQ_PH_ISFET_H

#include <stdint.h>
#include <stdbool.h>

/* API */
int ph_isfet_init(void);
int ph_isfet_read(float *ph, float *raw_mv);
int ph_isfet_calibrate(float ph_buffer_1, float ph_buffer_2);
int ph_isfet_get_calibration(float *offset, float *slope);

#endif /* FERMENTIQ_PH_ISFET_H */