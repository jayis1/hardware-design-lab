/*
 * power.h — Power management (STOP2, battery monitoring, regulator control)
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_POWER_H
#define DRIVERS_POWER_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize power management */
bool power_init(void);

/* Read battery voltage in mV */
uint16_t power_read_battery_mv(void);

/* Get battery percentage (0–100) */
uint8_t power_get_battery_pct(void);

/* Enter STOP2 low-power mode */
void power_enter_stop2(void);

/* Wake from STOP2 */
void power_wakeup(void);

/* Cleanup after STOP2 wakeup (restore clocks) */
void power_wakeup_cleanup(void);

/* Enable/disable LED power rail */
void power_enable_leds(bool on);

/* Enable/disable analog power rail */
void power_enable_analog(bool on);

/* Check if USB is powered (charging) */
bool power_is_charging(void);

/* Get device temperature (internal sensor, × 10 °C) */
int16_t power_read_temp_c_x10(void);

#endif /* DRIVERS_POWER_H */