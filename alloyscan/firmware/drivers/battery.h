/*
 * battery.h — Battery monitoring driver
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_BATTERY_H
#define ALLOYSCAN_BATTERY_H

#include <stdint.h>

/* Initialize battery ADC monitoring */
void battery_init(void);

/* Read battery voltage in millivolts */
uint16_t battery_read_mv(void);

/* Get battery percentage (0-100) */
uint8_t battery_percent(void);

/* Check if battery is low */
bool battery_is_low(void);

/* Check if battery is critical */
bool battery_is_critical(void);

#endif /* ALLOYSCAN_BATTERY_H */