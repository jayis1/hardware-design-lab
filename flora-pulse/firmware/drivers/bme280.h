/*
 * bme280.h — BME280 environmental sensor driver interface
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_BME280_H
#define FLORAPULSE_BME280_H

#include <stdint.h>

/* BME280 compensated data */
typedef struct {
    float temperature;  /* °C */
    float humidity;      /* % */
    float pressure;      /* Pa */
} bme280_data_t;

/* Initialize BME280 via I2C1.
 * Reads calibration coefficients from the sensor's NVM.
 */
void bme280_init(void);

/* Read compensated temperature/humidity/pressure */
void bme280_read(bme280_data_t *data);

/* Read temperature only (for fast loop) */
float bme280_read_temperature(void);

#endif /* FLORAPULSE_BME280_H */