/*
 * drivers/rtc.h — RV-3028-C7 RTC driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_RTC_H
#define FROSTSENTINEL_RTC_H

#include <stdint.h>

/* Initialize the RTC. On first power-up, sets a default time. */
int rtc_init(void);

/* Get current time as Unix epoch seconds (UTC). */
uint32_t rtc_get_seconds(void);

/* Set the RTC from a Unix epoch timestamp (UTC). */
void rtc_set_seconds(uint32_t epoch);

/* Set a countdown alarm N seconds from now (for Stop2 wake). */
void rtc_set_alarm(uint32_t seconds_from_now);

/* Check if the alarm has fired; clears the flag if so. Returns 1 if fired. */
int rtc_alarm_triggered(void);

#endif /* FROSTSENTINEL_RTC_H */