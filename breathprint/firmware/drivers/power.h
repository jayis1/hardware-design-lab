/*
 * power.h — Power management header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include "sensor_array.h"

void power_init(void);
uint8_t power_read_battery(void);
uint8_t power_is_charging(void);
float power_get_battery_voltage(void);
uint8_t power_is_critical(void);
void power_enter_sleep(void);
void power_enter_stop(void);
void power_enter_standby(void);
uint32_t power_estimate_remaining_min(void);
uint32_t power_estimate_samples_remaining(void);

#endif /* POWER_H */