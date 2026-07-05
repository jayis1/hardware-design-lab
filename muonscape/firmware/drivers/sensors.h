/*
 * sensors.h — environmental sensors interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_SENSORS_H
#define MUONSCAPE_DRV_SENSORS_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float temp_c;        /* BME280 temperature, °C       */
    float pressure_hpa;  /* BME280 pressure, hPa         */
    float humidity_pct;  /* BME280 humidity, %           */
    float accel_g[3];    /* LSM6DSO accel, g              */
    float gyro_dps[3];   /* LSM6DSO gyro, deg/s          */
    float roll_deg;      /* computed roll (device tilt)  */
    float pitch_deg;     /* computed pitch               */
    uint16_t lux;        /* LTR-329 ambient light, lux   */
} env_state_t;

muon_status_t sensors_init(void);
muon_status_t sensors_read(env_state_t *st);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_SENSORS_H */