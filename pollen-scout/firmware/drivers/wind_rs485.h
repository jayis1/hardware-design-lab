/*
 * wind_rs485.h - Wind sensor driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef WIND_RS485_H
#define WIND_RS485_H
#include <stdint.h>

typedef struct {
    float speed_mps;   /* wind speed, m/s     */
    float dir_deg;     /* wind direction, deg */
} wind_data_t;

int wind_init(void);
int wind_read(wind_data_t *out);

#endif /* WIND_RS485_H */