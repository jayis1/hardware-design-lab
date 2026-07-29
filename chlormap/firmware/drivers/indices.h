/*
 * indices.h — Chlorophyll/NDVI/NSI/LWBI index calculations
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_INDICES_H
#define DRIVERS_INDICES_H

#include <stdint.h>
#include "spectrometer.h"

/* Computed vegetation indices */
typedef struct {
    int16_t spad;          /* SPAD-equivalent chlorophyll (0–100) */
    int16_t ndvi_x1000;    /* NDVI × 1000 (-200 to 1000) */
    int16_t nsi_x1000;     /* Nitrogen sufficiency index × 1000 */
    int16_t lwbi_x1000;    /* Leaf water band index × 1000 */
    int16_t rededge_x1000; /* Red-edge slope × 1000 (nm⁻¹) */
    int16_t pri_x1000;     /* Photochemical reflectance index × 1000 */
    int16_t car_x1000;     /* Carotenoid/chlorophyll ratio × 1000 */
    int16_t temp_c_x10;    /* Device temperature × 10 (°C) */
} indices_t;

/* Compute all indices from spectrometer result */
void indices_compute(const spectrometer_result_t *spec, indices_t *idx);

/* Individual index calculations (exposed for testing) */
int16_t compute_spad(int16_t r660, int16_t r940);
int16_t compute_ndvi(int16_t r800, int16_t r660);
int16_t compute_nsi(int16_t r531, int16_t r570);
int16_t compute_lwbi(int16_t r900, int16_t r970);
int16_t compute_rededge(int16_t r700, int16_t r740);
int16_t compute_pri(int16_t r531, int16_t r570);
int16_t compute_car_ratio(int16_t r510, int16_t r680);

#endif /* DRIVERS_INDICES_H */