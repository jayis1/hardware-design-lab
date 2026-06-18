/*
 * bme280.h - BME280 T/RH/P driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef BME280_H
#define BME280_H
#include <stdint.h>

typedef struct {
    float temp_c;    /* °C   */
    float pres_pa;   /* Pa   */
    float rh_pct;    /* %RH  */
} bme280_data_t;

int bme280_init(void);
int bme280_read(bme280_data_t *out);

#endif /* BME280_H */