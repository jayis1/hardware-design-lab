/*
 * optflow.h — PMW3360 optical flow driver for drift correction
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_OUTFLOW_H
#define INKWELL_DRIVERS_OUTFLOW_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int16_t  dx_counts;
    int16_t  dy_counts;
    uint8_t  squal;        /* surface quality 0-255 */
    uint16_t shutter;      /* exposure auto-adjust */
} optflow_sample_t;

void optflow_init(void);
bool optflow_read(optflow_sample_t *out);
void optflow_set_cpi(uint16_t cpi);
void optflow_power_down(void);

#endif