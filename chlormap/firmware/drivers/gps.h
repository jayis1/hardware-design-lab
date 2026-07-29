/*
 * gps.h — u-blox NEO-M9N GNSS driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_GPS_H
#define DRIVERS_GPS_H

#include <stdint.h>
#include <stdbool.h>

/* GPS fix data */
typedef struct {
    int32_t  lat_e7;      /* latitude  × 1e-7 degrees */
    int32_t  lon_e7;      /* longitude × 1e-7 degrees */
    uint32_t itow;        /* GPS time of week (ms) */
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  fix_type;    /* 0=none, 2=2D, 3=3D */
    uint8_t  sats;        /* number of satellites used */
    uint32_t hacc_mm;     /* horizontal accuracy (mm) */
} gps_data_t;

/* Initialize GPS module over I2C */
bool gps_init(void);

/* Enable GPS (power on, start tracking) */
bool gps_enable(void);

/* Disable GPS (power save / backup mode) */
bool gps_disable(void);

/* Read latest NAV-PVT data */
bool gps_read(gps_data_t *data);

/* Check if GPS has a valid fix */
bool gps_has_fix(void);

/* Set update rate (1–10 Hz) */
bool gps_set_update_rate(uint8_t hz);

/* Get number of satellites in view */
uint8_t gps_get_sats(void);

#endif /* DRIVERS_GPS_H */