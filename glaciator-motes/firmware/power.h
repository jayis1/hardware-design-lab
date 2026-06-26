/*
 * power.h — BQ25895 charger, fuel gauge, solar MPPT, power FSM
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PWR_STATE_NORMAL = 0,    /* full seismic + mesh operation */
    PWR_STATE_DEGRADED,      /* reduced sample rate, GPS off */
    PWR_STATE_SURVIVAL,      /* polar night: minimal beacon only */
    PWR_STATE_CHARGE_ONLY,   /* no sampling, charging from solar */
    PWR_STATE_FAULT
} power_state_t;

void         power_init(void);
power_state_t power_state(void);
uint16_t     power_vbat_mv(void);
uint16_t     power_vsol_mv(void);
int16_t      power_temp_c(void);     /* 0.1 C units? No, °C */
uint8_t      power_solar_pct(void);  /* 0-100 */
bool         power_charging(void);
bool         power_charge_done(void);
void         power_tick_1hz(void);
void         power_set_heater(bool on);
const char  *power_state_name(power_state_t s);

#endif /* POWER_H */