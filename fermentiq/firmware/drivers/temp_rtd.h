/*
 * temp_rtd.h — MAX31865 PT100 RTD + SHT41 Ambient Driver Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_TEMP_RTD_H
#define FERMENTIQ_TEMP_RTD_H

#include <stdint.h>

/* API */
int temp_rtd_init(void);
int temp_rtd_read(float *temp_c);
int temp_ambient_read(float *temp_c, float *rh_pct);

#endif /* FERMENTIQ_TEMP_RTD_H */