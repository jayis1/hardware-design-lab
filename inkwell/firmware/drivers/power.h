/*
 * power.h — Power management: fuel gauge, charge state, sleep FSM
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_POWER_H
#define INKWELL_DRIVERS_POWER_H

#include <stdint.h>
#include <stdbool.h>

void     power_init(void);
uint8_t  power_get_battery_pct(void);
uint16_t power_get_battery_mv(void);
bool     power_is_charging(void);
void     power_enter_off(void);

#endif /* INKWELL_DRIVERS_POWER_H */