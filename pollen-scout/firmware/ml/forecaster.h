/*
 * forecaster.h - Pollen concentration forecaster header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef FORECASTER_H
#define FORECASTER_H
#include <stdint.h>

#define FORECAST_TAXA  6
#define FORECAST_HORIZON 72

typedef struct {
    float forecast[FORECAST_TAXA][FORECAST_HORIZON]; /* grains/m³ */
    int   valid_steps;
} forecaster_result_t;

int forecaster_init(void);
int forecaster_update(const float *concentration, float temp_c,
                      float rh_pct, float wind_mps, float wind_dir,
                      forecaster_result_t *out);

#endif /* FORECASTER_H */