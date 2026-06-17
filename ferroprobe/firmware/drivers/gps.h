/*
 * gps.h — NEO-M9N GNSS parser interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_GPS_H
#define FERROPROBE_GPS_H

#include <stdint.h>

typedef struct {
    int32_t latitude_e7;    /* ×10^7 (e.g., 476064000 = 47.6064°) */
    int32_t longitude_e7;    /* ×10^7 */
    int32_t altitude_mm;      /* mm above WGS84 ellipsoid */
    uint16_t speed_mm_s;     /* mm/s ground speed */
    uint16_t heading_deg10;   /* heading ×10, 0-3599 */
    uint8_t satellites;      /* number of sats in fix */
    uint8_t fix_quality;     /* 0=no fix, 1=GPS, 2=DGPS, 4=RTK fixed */
    uint8_t hour, minute, second;  /* UTC time */
    uint16_t year;
    uint8_t month, day;
    uint8_t valid;
} gps_data_t;

void gps_init(void);
void gps_poll(gps_data_t *data);
uint8_t gps_has_fix(void);
void gps_get_latest(gps_data_t *data);

#endif /* FERROPROBE_GPS_H */