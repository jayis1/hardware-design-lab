/*
 * bq25895.h - BQ25895 MPPT charger driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef BQ25895_H
#define BQ25895_H
#include <stdint.h>

typedef struct {
    uint16_t bat_mv;     /* battery voltage, mV    */
    uint16_t vin_mv;     /* solar input voltage, mV */
    uint16_t ichg_ma;    /* charge current, mA      */
    uint8_t  charge_pct; /* 0..100                   */
    uint8_t  charging;
    uint8_t  charge_done;
} bq25895_status_t;

int bq25895_init(void);
int bq25895_read(bq25895_status_t *out);

#endif /* BQ25895_H */