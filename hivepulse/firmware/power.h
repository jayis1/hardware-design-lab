/*
 * power.h — Power management API for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef POWER_H
#define POWER_H

#include <stdint.h>
#include <stdbool.h>

/* Power modes */
typedef enum {
    POWER_MODE_ACTIVE = 0,    /* Full power: audio, all sensors, radio */
    POWER_MODE_LOWPOWER = 1,  /* Reduced sampling, sensors duty-cycled */
    POWER_MODE_STANDBY = 2,   /* Only RTC + LoRa RX ping slots */
    POWER_MODE_SHUTDOWN = 3,  /* Deep sleep, RTC wakeup only */
} power_mode_t;

/* Initialize power management */
int power_management_init(void);

/* Read battery voltage, solar voltage, and charge current */
int power_read_battery(float *batt_mv, float *solar_mv, float *charge_ma);

/* Set power mode */
void power_set_mode(power_mode_t mode);

/* Initialize independent watchdog (IWDG) */
void power_watchdog_init(uint32_t timeout_ms);

/* Refresh watchdog */
void power_watchdog_refresh(void);

/* Initialize RTC (for timekeeping and wakeup alarms) */
int power_rtc_init(void);

/* Set RTC wakeup alarm (seconds from now) */
void power_set_rtc_wakeup(uint32_t seconds);

/* Save state to RTC backup registers (survives deep sleep/reset) */
int power_save_state_to_rtc(const void *state, uint32_t size);

/* Restore state from RTC backup registers */
int power_restore_state_from_rtc(void *state, uint32_t size);

/* Get RTC time as Unix timestamp */
uint32_t power_get_rtc_timestamp(void);

#endif /* POWER_H */