/*
 * drivers/thermo.h — Auxiliary sensor driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_THERMO_H
#define FROSTSENTINEL_THERMO_H

#include <stdint.h>

/* SHT45: measure air temperature (0.01 °C) and relative humidity (0.001). */
int sht45_measure(int32_t *temp_cx100, int32_t *rh_x1000);

/* BMP390: initialize and read pressure (hPa) + temperature (0.01 °C). */
int bmp390_init(void);
int bmp390_read(int32_t *pressure_hpa, int32_t *temp_cx100);

/* DS18B20: read leaf-surface temperature (0.01 °C) via 1-wire. */
int ds18b20_read_cx100(int32_t *temp_cx100);

/* Busy-wait microsecond delay (for 1-wire timing). */
void delay_us(uint32_t us);

#endif /* FROSTSENTINEL_THERMO_H */