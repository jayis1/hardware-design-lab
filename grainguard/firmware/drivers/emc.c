/*
 * emc.c — Equilibrium Moisture Content computation
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * Modified Henderson equation:
 *
 *        1
 *   MC = ─── × [ -ln(1 - RH) ]^(1/n)
 *        K × A
 *
 * Where RH is fractional (0..1), and K, A, n are grain-specific.
 * Temperature correction is applied to K:
 *   K_T = K × (1 + 0.01 × (T - 25))  (approximate Henderson temp correction)
 */

#include "emc.h"
#include "../board.h"
#include <math.h>

/* Grain parameters: { K, A, n, safe_MC_pct } */
static const struct {
    const char *name;
    float K;
    float A;
    float n;
    float safe_mc_pct;
} grain_params[GRAIN_COUNT + 1] = {
    { "Unknown", 0.0f,  1.0f, 5.0f, 14.0f },  /* index 0 unused */
    { "Wheat",   0.00046f, 1.0f, 5.3f, 13.5f },
    { "Corn",    0.00086f, 1.0f, 4.2f, 15.5f },
    { "Barley",  0.00051f, 1.0f, 5.0f, 14.0f },
    { "Rice",    0.00048f, 1.0f, 5.5f, 13.0f },
    { "Oats",    0.00063f, 1.0f, 4.6f, 14.0f },
    { "Soybean", 0.00200f, 1.0f, 3.0f, 13.0f },
};

int32_t emc_compute(uint8_t grain_type, int16_t temp_c_x10, int16_t rh_x100) {
    if (grain_type < 1 || grain_type > GRAIN_COUNT) return -1;

    float K = grain_params[grain_type].K;
    float A = grain_params[grain_type].A;
    float n = grain_params[grain_type].n;

    /* RH fractional */
    float rh = (float)rh_x100 / 10000.0f;
    if (rh <= 0.0f) return 0;
    if (rh >= 0.999f) rh = 0.999f;

    /* Temperature in Celsius */
    float T = (float)temp_c_x10 / 10.0f;

    /* Temperature-corrected K (Henderson) */
    float K_T = K * (1.0f + 0.01f * (T - 25.0f));
    if (K_T <= 0.0f) K_T = K;

    /* EMC in fraction */
    float mc_frac = (1.0f / (K_T * A)) * powf(-logf(1.0f - rh), 1.0f / n);

    /* Convert to percent × 1000 */
    int32_t mc_x1000 = (int32_t)(mc_frac * 100.0f * 1000.0f);
    if (mc_x1000 < 0)   mc_x1000 = 0;
    if (mc_x1000 > 50000) mc_x1000 = 50000;

    return mc_x1000;
}

int32_t emc_safe_threshold(uint8_t grain_type) {
    if (grain_type < 1 || grain_type > GRAIN_COUNT) return -1;
    return (int32_t)(grain_params[grain_type].safe_mc_pct * 1000.0f);
}

const char *emc_grain_name(uint8_t grain_type) {
    if (grain_type > GRAIN_COUNT) return "Unknown";
    return grain_params[grain_type].name;
}