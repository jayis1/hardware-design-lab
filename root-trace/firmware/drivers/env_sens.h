/*
 * env_sens.h — Environmental Sensors (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_ENV_SENS_H
#define ROOTTRACE_ENV_SENS_H

#include <stdint.h>

typedef struct {
    float soil_moisture_pct;   /* Volumetric water content (0-100%) */
    float soil_temp_c;         /* Soil temperature (°C) */
    float ambient_temp_c;      /* Air temperature (°C) */
    float ambient_rh;          /* Relative humidity (%) */
    uint32_t rtc_unix_time;    /* RTC time (Unix timestamp) */
} env_data_t;

void env_sens_init(void);
int  env_sens_read_all(env_data_t *data);
int  env_sens_read_soil_moisture(float *pct);
int  env_sens_read_soil_temp(float *celsius);
int  env_sens_read_ambient(float *temp_c, float *rh);
int  env_sens_rtc_get(uint32_t *unix_time);
int  env_sens_rtc_set(uint32_t unix_time);

#endif /* ROOTTRACE_ENV_SENS_H */