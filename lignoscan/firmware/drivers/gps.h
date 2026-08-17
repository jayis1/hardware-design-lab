/*
 * gps.h — u-blox NEO-M9N GPS Driver
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_GPS_H
#define LIGNOSCAN_GPS_H

#include "ble.h"  /* gps_fix_t defined here */

void gps_init(void);
int gps_has_fix(void);
void gps_get_fix(gps_fix_t *fix);
void gps_format_timestamp(gps_fix_t *fix, char *buf, int bufsize);
void gps_parse_nmea(const char *sentence);

#endif /* LIGNOSCAN_GPS_H */