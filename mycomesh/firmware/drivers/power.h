/*
 * power.h — Power management header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_POWER_H
#define MYCOMESH_POWER_H

#include <stdint.h>

/* Initialize power management. */
void power_init(void);

/* Power-gate the analog front end (ADS1298 rails). */
void power_disable_analog(void);

/* Enable the analog front end power rail. */
void power_enable_analog(void);

/* Power-gate environmental sensors. */
void power_disable_env(void);

/* Enable environmental sensor power rail. */
void power_enable_env(void);

/* Power-gate the microSD card. */
void power_disable_sd(void);

/* Enable microSD power rail. */
void power_enable_sd(void);

/* Enter STOP2 low-power mode for the specified duration.
 * RTC wake-up timer is configured to wake after ms milliseconds. */
void power_enter_stop2(uint32_t ms);

/* Read battery voltage in millivolts (via ADC). */
uint16_t power_read_battery_mv(void);

/* Read supercapacitor voltage in millivolts. */
uint16_t power_read_supercap_mv(void);

/* Check if solar input is present. */
uint8_t power_solar_present(void);

/* Get estimated remaining battery percentage (0-100). */
uint8_t power_battery_pct(void);

#endif /* MYCOMESH_POWER_H */