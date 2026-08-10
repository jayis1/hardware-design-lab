/**
 * @file    power.h
 * @brief   TideBand — Power management and battery fuel gauge driver API.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_POWER_H
#define TIDEBAND_POWER_H

#include <stdint.h>

/* ---- Public API ---- */

/** Initialize MAX17055 fuel gauge. */
void power_init(void);

/** Get battery state of charge (0-100%). */
float power_get_battery_pct(void);

/** Get battery voltage in volts. */
float power_get_voltage(void);

/** Get estimated time to empty in seconds (0 if charging/unavailable). */
uint32_t power_get_time_to_empty(void);

/** Enter low-power stop mode. Wakes on BLE or timer interrupt. */
void power_enter_stop(void);

/** Exit stop mode and restore clocks. */
void power_exit_stop(void);

/** Check if battery is critically low (< 10%). */
uint8_t power_is_critical(void);

/** Update fuel gauge readings — call periodically. */
void power_update(void);

#endif /* TIDEBAND_POWER_H */