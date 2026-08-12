/*
 * humid.h — SHT45 relative humidity + temperature driver (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_HUMID_H
#define GRAINGUARD_HUMID_H

#include <stdint.h>

typedef struct {
    int16_t  temperature_x100;   /* °C × 100 */
    int16_t  humidity_x100;      /* %RH × 100 */
} humid_meas_t;

int  humid_init(void);
int  humid_measure(humid_meas_t *out);

#endif /* GRAINGUARD_HUMID_H */