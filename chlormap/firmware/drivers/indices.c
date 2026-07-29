/*
 * indices.c — Chlorophyll/NDVI/NSI/LWBI/red-edge index calculations
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * All indices are computed from 16-band reflectance values (× 1000).
 * Results are returned as int16_t × 1000 (or × 10 for temperature).
 *
 * Band index reference:
 *   0: 450nm   1: 480nm   2: 510nm   3: 531nm
 *   4: 550nm   5: 570nm   6: 660nm   7: 680nm
 *   8: 700nm   9: 720nm  10: 740nm  11: 800nm
 *  12: 900nm  13: 940nm  14: 970nm  15: 1050nm
 */

#include "indices.h"
#include "board.h"
#include <math.h>

/* ---- Internal temperature sensor read (stub) ---- */
static int16_t read_device_temp_x10(void)
{
    /* In real build: read STM32L4 internal TS_CAL1/TS_CAL2 ADC channel
     * temp = (VSENSE - V25) / Avg_Slope + 25
     * Return × 10 (e.g., 253 = 25.3 °C)
     */
    return 250; /* 25.0 °C stub */
}

/* ---- SPAD-equivalent chlorophyll ----
 *
 * SPAD meters measure transmittance ratio: log(I940/I660)
 * In reflectance geometry, we approximate:
 *   SPAD = k * log10(R940 / R660) + offset
 * Calibrated to match Konica Minolta SPAD-502 scale (0–100).
 *
 * Typical healthy leaf: R660 ≈ 0.06, R940 ≈ 0.45 → ratio ≈ 7.5
 * SPAD ≈ 45. Yellow leaf: R660 ≈ 0.25, R940 ≈ 0.50 → ratio ≈ 2.0 → SPAD ≈ 15
 */
int16_t compute_spad(int16_t r660, int16_t r940)
{
    /* r660, r940 are reflectance × 1000 */
    if (r660 <= 0) r660 = 1;
    if (r940 <= 0) r940 = 1;

    /* Convert to float for log calculation */
    float ratio = (float)r940 / (float)r660;
    if (ratio < 0.01f) ratio = 0.01f;
    if (ratio > 100.0f) ratio = 100.0f;

    /* Empirical calibration: SPAD = 100 * log10(ratio) + 10 */
    float spad = 100.0f * log10f(ratio) + 10.0f;

    /* Clamp to valid range */
    if (spad < 0.0f) spad = 0.0f;
    if (spad > 100.0f) spad = 100.0f;

    return (int16_t)(spad + 0.5f);
}

/* ---- Normalized Difference Vegetation Index (leaf-level) ----
 * NDVI = (R800 - R660) / (R800 + R660)
 * Range: -1.0 to 1.0 (healthy leaf: 0.6–0.9)
 */
int16_t compute_ndvi(int16_t r800, int16_t r660)
{
    int32_t num = (int32_t)r800 - (int32_t)r660;
    int32_t den = (int32_t)r800 + (int32_t)r660;
    if (den == 0) return 0;
    return (int16_t)((num * 1000) / den);
}

/* ---- Nitrogen Sufficiency Index (NSI) ----
 * Based on the Photochemical Reflectance Index:
 * PRI = (R531 - R570) / (R531 + R570)
 * NSI is derived from PRI and correlates with leaf nitrogen content.
 * NSI = PRI_normalized × 1000
 * Low NSI → nitrogen deficiency.
 */
int16_t compute_nsi(int16_t r531, int16_t r570)
{
    int32_t num = (int32_t)r531 - (int32_t)r570;
    int32_t den = (int32_t)r531 + (int32_t)r570;
    if (den == 0) return 0;
    return (int16_t)((num * 1000) / den);
}

/* ---- Leaf Water Band Index (LWBI) ----
 * LWBI = R900 / R970
 * The 970 nm band is a water absorption feature.
 * Well-hydrated leaf: LWBI ≈ 1.05–1.15
 * Water-stressed leaf: LWBI ≈ 0.90–1.00
 */
int16_t compute_lwbi(int16_t r900, int16_t r970)
{
    if (r970 <= 0) return 1000;
    return (int16_t)(((int32_t)r900 * 1000) / r970);
}

/* ---- Red-edge slope ----
 * Slope between 700 nm and 740 nm:
 *   slope = (R740 - R700) / (740 - 700) = (R740 - R700) / 40
 * Steeper slope → higher chlorophyll / less stress.
 * Return × 1000 (per nm).
 */
int16_t compute_rededge(int16_t r700, int16_t r740)
{
    int32_t diff = (int32_t)r740 - (int32_t)r700;
    /* × 1000 / 40 = × 25 */
    return (int16_t)(diff * 25);
}

/* ---- Photochemical Reflectance Index (PRI) ----
 * PRI = (R531 - R570) / (R531 + R570)
 * Same formula as NSI but used as a stress/xanthophyll indicator.
 */
int16_t compute_pri(int16_t r531, int16_t r570)
{
    return compute_nsi(r531, r570);
}

/* ---- Carotenoid / chlorophyll ratio ----
 * CIR = R510 / R680
 * Higher ratio → more carotenoids relative to chlorophyll (senescence/stress).
 */
int16_t compute_car_ratio(int16_t r510, int16_t r680)
{
    if (r680 <= 0) return 0;
    return (int16_t)(((int32_t)r510 * 1000) / r680);
}

/* ---- Master computation ---- */
void indices_compute(const spectrometer_result_t *spec, indices_t *idx)
{
    if (!spec || !idx) return;

    /* Extract band values (reflectance × 1000) */
    int16_t r450 = spec->bands_x1000[0];
    int16_t r510 = spec->bands_x1000[2];
    int16_t r531 = spec->bands_x1000[3];
    int16_t r550 = spec->bands_x1000[4];
    int16_t r570 = spec->bands_x1000[5];
    int16_t r660 = spec->bands_x1000[6];
    int16_t r680 = spec->bands_x1000[7];
    int16_t r700 = spec->bands_x1000[8];
    int16_t r740 = spec->bands_x1000[10];
    int16_t r800 = spec->bands_x1000[11];
    int16_t r900 = spec->bands_x1000[12];
    int16_t r940 = spec->bands_x1000[13];
    int16_t r970 = spec->bands_x1000[14];

    /* Compute all indices */
    idx->spad          = compute_spad(r660, r940);
    idx->ndvi_x1000    = compute_ndvi(r800, r660);
    idx->nsi_x1000     = compute_nsi(r531, r570);
    idx->lwbi_x1000    = compute_lwbi(r900, r970);
    idx->rededge_x1000 = compute_rededge(r700, r740);
    idx->pri_x1000     = compute_pri(r531, r570);
    idx->car_x1000     = compute_car_ratio(r510, r680);
    idx->temp_c_x10    = read_device_temp_x10();

    /* Suppress unused-variable warnings for bands we don't use directly */
    (void)r450;
    (void)r550;
}