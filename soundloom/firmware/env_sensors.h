/*
 * env_sensors.h — Environmental sensor driver public interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */
#ifndef SOUNDLOOM_ENV_H
#define SOUNDLOOM_ENV_H

#include <stdint.h>

#define ENV_DEPTH_SENSORS 4

typedef struct {
    float    moisture[2];      /* Teros-12 volumetric water content, 2 depths */
    float    temp_teros[2];    /* Teros-12 temperature °C */
    float    ec[2];            /* Teros-12 electrical conductivity µS/cm */
    float    air_temp;         /* SHT45 air temperature °C */
    float    air_rh;           /* SHT45 relative humidity % */
    uint16_t co2_ppm;          /* GMP343 CO2 ppm */
    float    depth_temp[ENV_DEPTH_SENSORS]; /* DS18B20 at 10/20/40/60 cm */
    uint16_t battery_mv;      /* battery voltage mV */
    uint16_t solar_mv;        /* solar panel voltage mV */
    uint32_t timestamp;       /* HAL_GetTick() at reading */
} env_data_t;

void env_init(void);
int  env_read_all(env_data_t *out);
const env_data_t* env_get_last(void);
uint32_t env_get_read_count(void);
uint8_t env_battery_percent(void);

#endif /* SOUNDLOOM_ENV_H */