/*
 * power.h — Battery & Power Management Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_POWER_H
#define FERMENTIQ_POWER_H

/* API */
int power_init(void);
int power_read_battery(float *soc_pct, float *voltage_mv);
int power_get_charge_status(bool *charging, bool *full);
int power_enter_low_power(void);

#endif /* FERMENTIQ_POWER_H */