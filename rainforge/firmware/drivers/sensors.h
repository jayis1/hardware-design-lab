/*
 * sensors.h — Environmental sensors (SHT45, BMP390, TSL2591)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef RAINFORGE_SENSORS_H
#define RAINFORGE_SENSORS_H

#include <stdint.h>

typedef struct {
    float temperature_c;    /* °C */
    float humidity_pct;     /* %RH */
    float pressure_pa;      /* Pa */
    float ambient_lux;      /* lux */
    uint8_t sht45_ok;
    uint8_t bmp390_ok;
    uint8_t tsl2591_ok;
} env_sensors_t;

void  sensors_init(void);
int   sensors_read_all(env_sensors_t *out);
void  sensors_heater_pulse(void);   /* SHT45 condensation removal */

/* Individual sensor reads */
int   sht45_read(float *temp_c, float *rh_pct);
int   bmp390_read(float *pressure_pa, float *temp_c);
int   tsl2591_read(float *lux);

#endif /* RAINFORGE_SENSORS_H */