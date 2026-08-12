/*
 * emc.h — Equilibrium Moisture Content computation (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_EMC_H
#define GRAINGUARD_EMC_H

#include <stdint.h>

/* Compute equilibrium moisture content (%) from temperature and RH.
 * Uses the modified Henderson equation with grain-specific constants.
 * grain_type: one of GRAIN_WHEAT, GRAIN_CORN, ...
 * Returns EMC in percent × 1000 (e.g. 13500 = 13.5%).
 */
int32_t emc_compute(uint8_t grain_type, int16_t temp_c_x10, int16_t rh_x100);

/* Returns the safe storage MC threshold × 1000 for a given grain type. */
int32_t emc_safe_threshold(uint8_t grain_type);

/* Returns the human-readable grain name. */
const char *emc_grain_name(uint8_t grain_type);

#endif /* GRAINGUARD_EMC_H */