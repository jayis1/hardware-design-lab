/*
 * power.h — Battery and power management interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_POWER_H
#define SPECKLEFLOW_POWER_H

#include <stdint.h>

/**
 * Initialize the MAX17048 fuel gauge.
 * @return 0 on success, -1 on I2C error, -2 on wrong version
 */
int power_init(void);

/**
 * Update battery readings (call periodically, ~1 Hz).
 */
void power_update(void);

/**
 * Get battery state-of-charge (0–100%).
 */
uint8_t power_get_battery_pct(void);

/**
 * Get battery voltage in millivolts.
 */
uint16_t power_get_battery_mv(void);

/**
 * Check if USB-C charger is connected and charging.
 */
int power_is_charging(void);

/**
 * Check if battery is below warning threshold.
 */
int power_is_low(void);

/**
 * Get charge/discharge rate (signed, %/hr × 10).
 */
uint16_t power_get_charge_rate(void);

#endif /* SPECKLEFLOW_POWER_H */