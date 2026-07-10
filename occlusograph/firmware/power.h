/*
 * power.h — Power management interface for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_POWER_H
#define OCCLUSOGRAPH_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* Initialize PMIC and charger. */
int  power_init(void);

/* Get battery voltage in millivolts. */
uint16_t power_battery_mv(void);

/* Get battery state-of-charge (0-100%). */
uint8_t  power_battery_pct(void);

/* True if currently charging (inductive). */
bool power_is_charging(void);

/* Enter low-power sleep mode. Wakes on IMU interrupt or BLE event. */
void power_sleep(void);

/* Resume from sleep. */
void power_wake(void);

/* Set a wake timer in seconds (for periodic bruxism scan in sleep). */
void power_set_wake_timer(uint32_t seconds);

/* Get chip temperature in milli-Celsius (for thermal protection). */
int32_t power_chip_temp_mc(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_POWER_H */