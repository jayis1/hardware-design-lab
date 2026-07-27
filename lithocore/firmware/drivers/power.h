/*
 * power.h — Power management (analog rail gating, STOP2, supercap)
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_POWER_H
#define LITHOCORE_POWER_H

#include <stdint.h>

int  power_init(void);
void power_analog_on(void);
void power_analog_off(void);
void power_supercap_charge_enable(void);
void power_supercap_charge_disable(void);
uint8_t power_supercap_is_ready(void);
void power_ble_wake(void);
void power_ble_sleep(void);
uint32_t power_get_battery_mv(void);

#endif /* LITHOCORE_POWER_H */