/*
 * power.h — Power management & RTC scheduler (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_POWER_H
#define GRAINGUARD_POWER_H

#include <stdint.h>

/* Initialize the RTC and power management. */
int  power_init(void);

/* Read the battery voltage in millivolts. */
uint16_t power_read_battery_mv(void);

/* Read the supercap voltage (for backup state). */
uint16_t power_read_supercap_mv(void);

/* Enter STOP2 low-power mode, waking via RTC wake-up timer.
 * sleep_seconds: how long to sleep. */
void power_enter_stop2(uint32_t sleep_seconds);

/* Schedule the next RTC wake-up. */
void power_schedule_wakeup(uint32_t seconds);

/* Check if we just woke from STOP2 (vs cold boot). */
int  power_woke_from_stop(void);

/* ---- RTC time helpers ---- */
uint32_t rtc_get_epoch_seconds(void);
void     rtc_set_epoch_seconds(uint32_t sec);

#endif /* GRAINGUARD_POWER_H */