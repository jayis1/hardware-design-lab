/*
 * power_drv.h — Power management driver header
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef TREMORSENSE_POWER_DRV_H
#define TREMORSENSE_POWER_DRV_H

#include <stdint.h>

int      power_init(void);
uint8_t  power_get_battery_percent(void);
uint16_t power_get_battery_mv(void);
uint8_t  power_is_charging(void);
int8_t   power_get_charge_current_ma(void);
uint16_t power_get_estimated_runtime_hours(void);
void     power_enter_deep_sleep(void);
void     power_wake_from_deep_sleep(void);
void     power_set_cpu_frequency_mhz(uint32_t mhz);
uint32_t power_get_cpu_frequency_mhz(void);
void     power_enable_peripheral(uint8_t peripheral_id);
void     power_disable_peripheral(uint8_t peripheral_id);
uint32_t power_get_current_draw_ua(void);

#endif /* TREMORSENSE_POWER_DRV_H */