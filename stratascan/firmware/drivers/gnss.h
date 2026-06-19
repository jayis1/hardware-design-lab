/*
 * gnss.h — SAM-M10Q GNSS Driver (NMEA Parser)
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_GNSS_H
#define STRATASCAN_GNSS_H

#include <stdint.h>

int   gnss_init(void);
void  gnss_get_position(float *lat, float *lon, float *alt);
uint8_t gnss_get_fix(void);
uint8_t gnss_get_sats(void);
void  gnss_reset(void);

#endif /* STRATASCAN_GNSS_H */