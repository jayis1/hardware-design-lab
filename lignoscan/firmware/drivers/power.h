/*
 * power.h — Power Management Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_POWER_H
#define LIGNOSCAN_POWER_H

#include <stdint.h>

void power_init(void);
uint8_t power_get_battery_percent(void);
void power_update_gauge(void);
void power_enter_sleep(void);
uint32_t power_idle_time_seconds(void);
int power_is_charging(void);

#endif /* LIGNOSCAN_POWER_H */