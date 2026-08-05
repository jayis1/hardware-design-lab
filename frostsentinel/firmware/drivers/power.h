/*
 * drivers/power.h — Power management header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_POWER_H
#define FROSTSENTINEL_POWER_H

#include <stdint.h>

/* Initialize the LC709203F battery fuel gauge. */
int power_gauge_init(void);

/* Read battery state of charge (%) and voltage (mV). */
int power_gauge_read(uint8_t *pct, uint16_t *mv);

/* Check if solar input is active. */
int power_check_solar(uint8_t *solar_good);

/* Enter Stop2 low-power mode for the given number of seconds.
 * Restores clocks and SPI on wake. */
void power_enter_stop2(uint32_t wake_in_seconds);

/* Update g_sys.battery_pct, g_sys.battery_mv, and power-related flags. */
void power_update_status(void);

#endif /* FROSTSENTINEL_POWER_H */