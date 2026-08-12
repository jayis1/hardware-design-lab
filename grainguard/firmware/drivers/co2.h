/*
 * co2.h — SCD41 NDIR CO2 sensor driver (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_CO2_H
#define GRAINGUARD_CO2_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t co2_ppm;     /* CO2 in ppm */
    int16_t  temperature; /* 0.01 °C */
    int16_t  humidity;    /* 0.01 %RH (from SCD41; cross-check with SHT45) */
} co2_meas_t;

/* Initialize the SCD41 (reinit + stop periodic mode; we use single-shot) */
int  co2_init(void);

/* Trigger a single-shot measurement. Returns 0 on success.
 * Measurement completes in ~5 s; caller should delay then call co2_read(). */
int  co2_trigger_single_shot(void);

/* Read the last single-shot result. Returns 0 on success. */
int  co2_read(co2_meas_t *out);

/* Convenience: trigger, wait, read (blocking, ~5 s). */
int  co2_measure_blocking(co2_meas_t *out);

/* Power down the sensor via the board supply gate. */
void co2_power_off(void);
void co2_power_on(void);

#endif /* GRAINGUARD_CO2_H */