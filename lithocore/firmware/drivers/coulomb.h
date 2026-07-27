/*
 * coulomb.h — Coulomb counter for capacity estimation
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_COULOMB_H
#define LITHOCORE_COULOMB_H

#include <stdint.h>

/* The capacity check is an optional partial-discharge measurement.
 * The user can initiate it from the app (it takes 10-60 minutes depending
 * on the C-rate and depth). LithoCore discharges at 0.5C for a configurable
 * time, Coulomb-counts the extracted charge, and extrapolates to full
 * capacity using the OCV-vs-SoC curve for the detected chemistry. */

typedef struct {
    uint32_t total_mah_discharged;  /* mAh extracted during test */
    uint32_t duration_ms;           /* test duration */
    uint16_t start_mv;              /* OCV at start */
    uint16_t end_mv;                /* OCV at end */
    uint16_t estimated_capacity_mah; /* extrapolated full capacity */
    uint8_t  valid;
} coulomb_result_t;

int coulomb_init(void);
int coulomb_start_discharge(uint16_t current_ma, uint32_t duration_ms);
int coulomb_get_result(coulomb_result_t *result);
void coulomb_abort(void);

#endif /* LITHOCORE_COULOMB_H */