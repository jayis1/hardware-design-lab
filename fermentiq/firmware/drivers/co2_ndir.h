/*
 * co2_ndir.h — Senseair S8 NDIR CO2 Sensor Driver Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_CO2_NDIR_H
#define FERMENTIQ_CO2_NDIR_H

#include <stdint.h>

/* API */
int co2_ndir_init(void);
int co2_ndir_read_ppm(uint16_t *ppm);
int co2_ndir_read_serial(uint32_t *serial);
int co2_ndir_read_version(uint16_t *version);
int co2_ndir_calibration(uint16_t target_ppm);

#endif /* FERMENTIQ_CO2_NDIR_H */