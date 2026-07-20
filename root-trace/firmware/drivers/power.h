/*
 * power.h — Power Management (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_POWER_H
#define ROOTTRACE_POWER_H

#include <stdint.h>

typedef struct {
    uint16_t voltage_mv;    /* Battery voltage (mV) */
    uint8_t  percent;       /* State of charge (%) */
    int8_t   temp_c;        /* Battery temperature (°C) */
    uint8_t  charging;      /* 1 if USB charging */
    uint8_t  usb_connected;  /* 1 if USB VBUS present */
} power_status_t;

void power_init(void);
int  power_read_status(power_status_t *status);
void power_enter_stop(void);
void power_wakeup(void);
int  power_is_charging(void);
void power_set_led(uint8_t r, uint8_t g, uint8_t b);
void power_enter_low_power(void);

#endif /* ROOTTRACE_POWER_H */