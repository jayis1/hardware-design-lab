/*
 * drivers/skyir.h — MLX90632 sky infrared radiometer driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_SKYIR_H
#define FROSTSENTINEL_SKYIR_H

#include <stdint.h>

/*
 * Read the effective sky radiating temperature.
 *
 * Returns 0 on success, negative on error.
 * On success, *sky_temp_cx100 holds the temperature in units of 0.01 °C
 * (e.g. -2350 means -23.50 °C).
 *
 * The sensor is woken, a single-shot measurement is triggered, the
 * data-ready flag is polled, the raw thermopile and die-temperature
 * counts are read, the factory EEPROM calibration constants are applied,
 * and the sensor is put back to sleep.
 */
int skyir_read_sky_temp_cx100(int32_t *sky_temp_cx100);

/* Power management */
int skyir_wake(void);
int skyir_sleep(void);

#endif /* FROSTSENTINEL_SKYIR_H */