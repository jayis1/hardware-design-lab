/*
 * env_sensors.h — Environmental sensors header for MycoMesh
 * MycoMesh — Open-Source Fungal Mycelium Electrophysiology & Chemical Sensing Network
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef MYCOMESH_ENV_SENSORS_H
#define MYCOMESH_ENV_SENSORS_H

#include <stdint.h>

/* Environmental data structure. */
typedef struct {
    int16_t  moisture_pct_x10;  /* % VWC × 10 (0–1000)        */
    int16_t  temp_cx10;         /* °C × 10 (-100 to 600)      */
    uint16_t co2_ppm;           /* CO₂ in ppm (400–5000)       */
    uint16_t humidity_pct;      /* relative humidity %         */
} env_data_t;

/* Initialize all environmental sensors (DS18B20, SCD41, SMT100). */
int env_sensors_init(void);

/* Read all environmental sensors into the provided structure. */
int env_sensors_read_all(env_data_t *env);

/* Read soil temperature from DS18B20 (1-Wire). */
int16_t env_read_temperature_ds18b20(void);

/* Read CO₂ from SCD41 (I²C, photoacoustic). */
uint16_t env_read_co2_scd41(void);

/* Read soil moisture from SMT100 (SDI-12). */
int16_t env_read_moisture_smt100(void);

/* Force CO₂ calibration (single-shot measurement). */
void env_scd41_single_shot(void);

#endif /* MYCOMESH_ENV_SENSORS_H */