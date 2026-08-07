/*
 * ad5933.h — AD5933 Impedance Analyzer Driver Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_AD5933_H
#define FERMENTIQ_AD5933_H

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Maximum sweep points */
#define AD5933_MAX_POINTS   100

typedef struct {
    float freq_hz[AD5933_MAX_POINTS];
    float real[AD5933_MAX_POINTS];
    float imag[AD5933_MAX_POINTS];
    float mag[AD5933_MAX_POINTS];
    float phase_rad[AD5933_MAX_POINTS];
    int num_points;
} ad5933_sweep_result_t;

/* API */
int ad5933_init(void);
float ad5933_calibrate(float reference_resistor);
int ad5933_run_sweep(ad5933_sweep_result_t *result);
void ad5933_extract_features(const ad5933_sweep_result_t *sweep,
                             impedance_data_t *data, float cal_factor);
int ad5933_read_temperature(float *temp_c);

#endif /* FERMENTIQ_AD5933_H */