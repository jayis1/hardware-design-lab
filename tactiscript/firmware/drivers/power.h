/*
 * power.h — TactiScript power management header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_POWER_H
#define TACTISCRIPT_POWER_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize power management: PMIC, fuel gauge, charging, SAADC */
void power_init(void);

/* Read battery percentage (0-100) */
uint8_t power_read_battery_pct(void);

/* Read battery voltage in millivolts */
uint16_t power_read_battery_mv(void);

/* Check if currently charging (Qi or USB) */
bool power_is_charging(void);

/* Pulse the LRA vibration motor for `duration_ms` milliseconds */
void power_lra_pulse(uint32_t duration_ms);

/* Set LRA vibration intensity (0-100 %) */
void power_lra_set_intensity(uint8_t percent);

/* Enable/disable LRA */
void power_lra_enable(bool enable);

/* Delay functions (busy-wait, used by drivers) */
void power_delay_ms(uint32_t ms);
void power_delay_us_stub(uint32_t us);

/* Enter deep sleep (lowest power, wake on IMU tap or BLE connect) */
void power_enter_deep_sleep(void);

/* Read NTC temperature in degrees Celsius (×10, e.g. 251 = 25.1°C) */
int16_t power_read_temperature(void);

/* Check skin contact (capacitive sensor) — true if ring is on finger */
bool power_check_skin_contact(void);

#endif /* TACTISCRIPT_POWER_H */