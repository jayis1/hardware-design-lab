/*
 * power_mgmt.h — Power management and solar harvesting
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <stdint.h>
#include <stdbool.h>

/* Power states */
typedef enum {
    POWER_STATE_ACTIVE = 0,  /* Full operation: thermal, IMU, BLE, LoRa */
    POWER_STATE_IDLE,        /* Thermal off, BLE on, IMU standby */
    POWER_STATE_SLEEP,       /* LoRa CAD only, RTC + IMU tap detect */
    POWER_STATE_SOLAR_SUSTAIN, /* Ultra-low power, solar charging only */
    POWER_STATE_FAULT,       /* Safety fault, minimal operation */
} power_state_t;

/* Initialize power management */
void power_init(void);

/* Get current power state */
power_state_t power_get_state(void);

/* Set desired power state (may not take effect immediately if charging) */
void power_set_state(power_state_t state);

/* Update power state based on battery, solar, and activity.
 * Called periodically (1 Hz) from main loop. */
void power_update(void);

/* Get battery info */
uint8_t  power_get_battery_pct(void);
uint16_t power_get_battery_mv(void);
int16_t  power_get_charge_rate_ma(void);

/* Get solar info */
bool     power_solar_active(void);
uint16_t power_get_solar_mv(void);

/* Check if USB-C is connected (charging) */
bool power_usb_connected(void);

/* Telemetry: fill into telemetry packet */
void power_fill_telemetry(uint8_t *battery_pct, uint16_t *solar_mv,
                           uint8_t *state);

#endif /* POWER_MGMT_H */