/*
 * spectrometer.c — 128-element spectral processing + 16-band binning
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * This module transforms raw 128-element ADC samples into calibrated
 * 16-band reflectance values. The pipeline is:
 *
 *  1. Dark subtraction (element-wise)
 *  2. Linearity correction (per-element factory coefficients)
 *  3. Wavelength mapping (element index → nm via grating dispersion)
 *  4. Gaussian-weighted binning into 16 target bands (±5 nm FWHM)
 *  5. Reflectance: R(λ) = sample / white_reference
 */

#include "spectrometer.h"
#include "calib.h"
#include "board.h"
#include <string.h>
#include <math.h>

/* ---- 16 target band center wavelengths (nm) ---- */
const uint16_t band_wavelengths[NUM_BANDS] = {
    450, 480, 510, 531, 550, 570, 660, 680,
    700, 720, 740, 800, 900, 940, 970, 1050
};

/* ---- Element-to-wavelength mapping ----
 * The grating disperses 450–1050 nm across 128 elements (element 0 = 450 nm,
 * element 127 = 1050 nm). Linear dispersion: (1050-450)/127 ≈ 4.72 nm/element.
 */
const uint16_t element_wavelengths[ARRAY_ELEMENTS] = {
    /* Computed at init for efficiency, but we can static-init */
    450, 455, 459, 464, 469, 474, 478, 483, 488, 493, 497, 502, 507, 512,
    516, 521, 526, 531, 535, 540, 545, 550, 554, 559, 564, 569, 574, 578,
    583, 588, 593, 597, 602, 607, 612, 616, 621, 626, 631, 636, 640, 645,
    650, 655, 659, 664, 669, 674, 678, 683, 688, 693, 698, 702, 707, 712,
    717, 721, 726, 731, 736, 741, 745, 750, 755, 760, 764, 769, 774, 779,
    783, 788, 793, 798, 803, 807, 812, 817, 822, 826, 831, 836, 841, 846,
    850, 855, 860, 865, 869, 874, 879, 884, 888, 893, 898, 903, 908, 912,
    917, 922, 927, 931, 936, 941, 946, 951, 955, 960, 965, 970, 974, 979,
    984, 989, 994, 998, 1003, 1008, 1013, 1017, 1022, 1027, 1032, 1037,
    1041, 1046, 1051
};

/* ---- Per-element linearity correction coefficients ----
 * Factory calibration: corrected = raw * gain[i] + offset[i]
 * In production these are stored in Flash; here we use unity gain + zero offset.
 */
static float g_elem_gain[ARRAY_ELEMENTS];
static int32_t g_elem_offset[ARRAY_ELEMENTS];

/* ---- White reference (dark-corrected, stored by calib module) ---- */
static int32_t g_white_ref[ARRAY_ELEMENTS];
static bool g_ref_loaded = false;

/* ---- Gaussian weight lookup ----
 * For each target band, we compute Gaussian weights for nearby elements.
 * FWHM = 10 nm → sigma = 10 / 2.355 ≈ 4.25 nm
 * Element spacing ≈ 4.72 nm, so ±2 elements on each side.
 */
#define GAUSS_FWHM_NM     10.0f
#define GAUSS_SIGMA_NM    (GAUSS_FWHM_NM / 2.355f)
#define GAUSS_HALF_WIDTH  3  /* elements on each side */

static float gauss_weight(float elem_nm, float band_nm)
{
    float d = elem_nm - band_nm;
    return expf(-(d * d) / (2.0f * GAUSS_SIGMA_NM * GAUSS_SIGMA_NM));
}

bool spectrometer_init(void)
{
    /* Initialize linearity coefficients to unity */
    for (int i = 0; i < ARRAY_ELEMENTS; i++) {
        g_elem_gain[i] = 1.0f;
        g_elem_offset[i] = 0;
    }

    /* Load white reference from calibration */
    if (calib_get_white_reference(g_white_ref)) {
        g_ref_loaded = true;
    }

    return true;
}

bool spectrometer_process(const int32_t *raw, const int32_t *dark,
                          spectrometer_result_t *result)
{
    if (!raw || !dark || !result) return false;

    /* 1. Dark subtraction + linearity correction */
    for (int i = 0; i < ARRAY_ELEMENTS; i++) {
        int32_t corrected = raw[i] - dark[i];
        corrected = (int32_t)(corrected * g_elem_gain[i]) + g_elem_offset[i];
        if (corrected < 0) corrected = 0;
        result->elements[i] = corrected;
    }

    /* 2. Gaussian-weighted binning into 16 bands */
    for (int b = 0; b < NUM_BANDS; b++) {
        float band_nm = (float)band_wavelengths[b];
        float weighted_sum = 0.0f;
        float weight_total = 0.0f;

        /* Find central element for this band */
        int center = -1;
        for (int i = 0; i < ARRAY_ELEMENTS; i++) {
            if (element_wavelengths[i] >= (uint16_t)band_nm) {
                center = i;
                break;
            }
        }
        if (center < 0) center = ARRAY_ELEMENTS - 1;

        /* Sum Gaussian-weighted contributions from nearby elements */
        int lo = center - GAUSS_HALF_WIDTH;
        int hi = center + GAUSS_HALF_WIDTH;
        if (lo < 0) lo = 0;
        if (hi >= ARRAY_ELEMENTS) hi = ARRAY_ELEMENTS - 1;

        for (int i = lo; i <= hi; i++) {
            float w = gauss_weight((float)element_wavelengths[i], band_nm);
            weighted_sum += (float)result->elements[i] * w;
            weight_total += w;
        }

        float band_value = (weight_total > 0.0f) ?
            (weighted_sum / weight_total) : 0.0f;

        result->raw_bands[b] = (int16_t)band_value;

        /* 3. Reflectance: R(λ) = sample / white_reference */
        if (g_ref_loaded) {
            /* Find reference value for this band (same Gaussian binning) */
            float ref_sum = 0.0f;
            float ref_wt = 0.0f;
            int clo = center - GAUSS_HALF_WIDTH;
            int chi = center + GAUSS_HALF_WIDTH;
            if (clo < 0) clo = 0;
            if (chi >= ARRAY_ELEMENTS) chi = ARRAY_ELEMENTS - 1;
            for (int i = clo; i <= chi; i++) {
                float w = gauss_weight((float)element_wavelengths[i], band_nm);
                ref_sum += (float)g_white_ref[i] * w;
                ref_wt += w;
            }
            float ref_val = (ref_wt > 0.0f) ? (ref_sum / ref_wt) : 1.0f;
            if (ref_val < 1.0f) ref_val = 1.0f; /* avoid div-by-zero */

            float reflectance = band_value / ref_val;
            if (reflectance > 2.0f) reflectance = 2.0f; /* clamp */
            if (reflectance < 0.0f) reflectance = 0.0f;

            result->bands_x1000[b] = (int16_t)(reflectance * 1000.0f);
        } else {
            /* No reference: output raw × 1000 / max */
            result->bands_x1000[b] = (int16_t)(band_value * 1000.0f /
                                               8388607.0f); /* 24-bit max */
        }
    }

    return true;
}

uint16_t spectrometer_band_wavelength(uint8_t band_idx)
{
    if (band_idx >= NUM_BANDS) return 0;
    return band_wavelengths[band_idx];
}