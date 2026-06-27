/*
 * env_drv.h — Environmental sensors driver header
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef TREMORSENSE_ENV_DRV_H
#define TREMORSENSE_ENV_DRV_H

#include <stdint.h>

typedef struct {
    float temperature_c;   /* Degrees Celsius */
    float humidity_pct;    /* Relative humidity % */
    float pressure_hpa;    /* Barometric pressure hPa */
    float altitude_m;      /* Estimated altitude (meters) */
} env_data_t;

int  env_init(void);
void env_read_all(env_data_t *data);
float env_get_temperature(void);
float env_get_humidity(void);
float env_get_pressure(void);
float env_get_altitude(void);
uint8_t env_get_sht40_serial(uint32_t *serial);

#endif /* TREMORSENSE_ENV_DRV_H */