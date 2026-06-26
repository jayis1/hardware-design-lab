/*
 * gps_discipline.h — NEO-M9N GPS UART + 1-PPS PLL for time sync
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef GPS_DISCIPLINE_H
#define GPS_DISCIPLINE_H

#include <stdint.h>
#include <stdbool.h>

void   gps_init(void);
void   gps_power_on(void);
void   gps_power_off(void);
bool   gps_is_fixed(void);
uint32_t gps_utc_ms(void);            /* current UTC, ms */
int32_t gps_pps_offset_us(void);      /* local clock - UTC, µs (after PPS) */
void   gps_pps_isr(void);             /* EXTI3 handler */
void   gps_uart_isr(void);            /* USART1 RX handler */
void   gps_tick_1hz(void);            /* called from RTC 1 Hz */
void   gps_parse_nmea(const char *s); /* NMEA sentence parser */
float  gps_lat(void);
float  gps_lon(void);
float  gps_hdop(void);

#endif /* GPS_DISCIPLINE_H */