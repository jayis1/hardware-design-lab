/*
 * drivers/psychro.h — Psychrometer (wet-bulb) driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_PSYCHRO_H
#define FROSTSENTINEL_PSYCHRO_H

#include <stdint.h>

/*
 * Run a full psychrometer cycle: fan on for 8 s, sample both RTDs,
 * compute dry-bulb, wet-bulb, and wet-bulb depression.
 *
 * All temperatures are in units of 0.01 °C.
 * wick_dry is set to 1 if the wet-bulb depression is < 0.20 °C,
 * indicating the wick has run dry and the wet-bulb reading is invalid.
 */
int psychro_measure(int32_t *tdry_cx100, int32_t *twet_cx100,
                    int32_t *depression_cx100, uint8_t *wick_dry);

/*
 * Compute the dew point from dry-bulb, wet-bulb, and station pressure
 * using the psychrometric equation and the Magnus formula.
 *
 * tdry_cx100, twet_cx100: temperatures in 0.01 °C
 * pressure_hpa: station pressure in hPa
 * Returns dew point in 0.01 °C.
 */
int32_t psychro_dewpoint_cx100(int32_t tdry_cx100, int32_t twet_cx100,
                               int32_t pressure_hpa);

#endif /* FROSTSENTINEL_PSYCHRO_H */