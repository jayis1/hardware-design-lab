/*
 * sensors.h — AeroCast environment & wind sensors API
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef AEROCAST_SENSORS_H
#define AEROCAST_SENSORS_H

#include <stdint.h>

void sensors_init(void);
int  sensors_read_env(float *t_c, float *rh, float *p_hpa);
int  sensors_read_wind(float *dir_deg, float *speed_ms);

#endif /* AEROCAST_SENSORS_H */