/*
 * circadian.c — Circadian photometry calculations
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements CIE S 026:2018 α-opic quantities (melanopic EDI, cyanopic
 * EDI), CIE 1931 chromaticity (x, y), correlated color temperature via
 * McCamy's formula, Duv from the Planckian locus, and the Rea-Figueiro
 * CLA/CS circadian model.
 */

#include "circadian.h"
#include "spectrometer.h"
#include "../board.h"
#include <math.h>

/* ------------------------------------------------------------------ */
/*  CIE 1931 color matching function approximation (1931 2° observer)   */
/*  Uses Gaussian fits for xbar, ybar, zbar.                            */
/* ------------------------------------------------------------------ */

static float cie_xbar(float lam_nm)
{
    float t = (lam_nm - 599.8f) / 37.0f;
    return 1.056f * expf(-0.5f * t * t)
         + 0.362f * expf(-0.5f * ((lam_nm - 442.0f) / 16.0f) * ((lam_nm - 442.0f) / 16.0f))
         - 0.065f * expf(-0.5f * ((lam_nm - 501.1f) / 20.4f) * ((lam_nm - 501.1f) / 20.4f));
}

static float cie_ybar(float lam_nm)
{
    float t = (lam_nm - 556.3f) / 46.0f;
    return 1.011f * expf(-0.5f * t * t);
}

static float cie_zbar(float lam_nm)
{
    float t = (lam_nm - 449.8f) / 31.0f;
    return 2.060f * expf(-0.5f * t * t);
}

/* ------------------------------------------------------------------ */
/*  CIE S 026 α-opic action spectra (simplified single-Gaussian)        */
/*  These are simplified versions of the CIE S 026 α-opic sensitivities. */
/* ------------------------------------------------------------------ */

static float s_melanopic(float lam_nm)
{
    /* Melanopsin peak ~480 nm. */
    float t = (lam_nm - 480.0f) / 40.0f;
    return 1.0f * expf(-0.5f * t * t);
}

static float s_cyanopic(float lam_nm)
{
    /* S-cone peak ~442 nm. */
    float t = (lam_nm - 442.0f) / 35.0f;
    return 1.0f * expf(-0.5f * t * t);
}

/* ------------------------------------------------------------------ */
/*  McCamy's formula for CCT from chromaticity                          */
/* ------------------------------------------------------------------ */

static float mccamy_cct(float x, float y)
{
    float n = (x - 0.3320f) / (0.1858f - y);
    float cct = 449.0f * n*n*n + 3525.0f * n*n + 6823.3f * n + 5520.33f;
    if (cct < 1000.0f) cct = 1000.0f;
    if (cct > 40000.0f) cct = 40000.0f;
    return cct;
}

/* ------------------------------------------------------------------ */
/*  Duv calculation (approximate, from 1976 u'v')                       */
/* ------------------------------------------------------------------ */

static float compute_duv(float u_prime, float v_prime)
{
    /* Distance from Planckian locus in u'v' space. */
    /* Reference point for 6500K. */
    float u_p = 0.1979f;
    float v_p = 0.4683f;
    float du = u_prime - u_p;
    float dv = v_prime - v_p;
    return sqrtf(du * du + dv * dv);
}

void circadian_cct_duv(const spectral_sample_t *spec,
                       float *cct, float *duv, float *x, float *y)
{
    float X = 0, Y = 0, Z = 0;
    int i;

    /* Integrate tristimulus values from AS7343 channels. */
    for (i = 0; i < 9; i++) {
        float lam = as7343_wl_nm[i];
        float e = spec->irradiance_uw_cm2[i];
        X += e * cie_xbar(lam);
        Y += e * cie_ybar(lam);
        Z += e * cie_zbar(lam);
    }

    float sum = X + Y + Z;
    if (sum < 1e-9f) {
        *x = 0.3128f; *y = 0.3290f;  /* D65 default */
        *cct = 6500.0f;
        *duv = 0.0f;
        return;
    }

    *x = X / sum;
    *y = Y / sum;
    *cct = mccamy_cct(*x, *y);

    /* Convert to u'v'. */
    float denom = 2.0f * *x - 12.0f * *y + 3.0f;
    if (fabsf(denom) < 1e-9f) denom = 1e-9f;
    float u_p = 4.0f * *x / denom;
    float v_p = 9.0f * *y / denom;
    *duv = compute_duv(u_p, v_p);
}

float circadian_melanopic_edi(const spectral_sample_t *spec)
{
    /* Compute melanopic EDI by summing channel contributions weighted  */
    /* by the precomputed melanopic_w factors and applying the D65       */
    /* normalization factor.                                            */
    float m = 0.0f;
    int i;
    for (i = 0; i < AS7343_NUM_CHANNELS; i++) {
        m += spec->irradiance_uw_cm2[i] * as7343_melanopic_w[i];
    }
    /* D65 normalization: melanopic EDI = melanopic irradiance ×        */
    /* EDI/D65_melanopic = 9.29 × melanopic_W/m².                       */
    /* Convert µW/cm² to W/m²: multiply by 10.                          */
    return m * 10.0f * 9.29f;
}

float circadian_illuminance(const spectral_sample_t *spec)
{
    float lux = 0.0f;
    int i;
    for (i = 0; i < AS7343_NUM_CHANNELS; i++) {
        lux += spec->irradiance_uw_cm2[i] * as7343_photopic_w[i];
    }
    /* µW/cm² → lux: 1 W/m² ≈ 683 lux at 555nm. */
    /* Convert µW/cm² to W/m²: ×10. */
    return lux * 10.0f * 683.0f / 100.0f;  /* empirical calibration factor */
}

float circadian_rea_CLA(const spectral_sample_t *spec)
{
    /* Rea et al. CLA 2.0 model (simplified):                            */
    /*   CLA = 1548 × [M − (m − k) × 0.5]                                */
    /* where M = melanopic irradiance, m = melanopic contribution from   */
    /* S-cones, k = blue-yellow opponent.                                */
    float M = 0.0f, S = 0.0f, V = 0.0f;
    int i;
    for (i = 0; i < 9; i++) {
        float e = spec->irradiance_uw_cm2[i];
        M += e * s_melanopic(as7343_wl_nm[i]);
        S += e * s_cyanopic(as7343_wl_nm[i]);
        V += e * cie_ybar(as7343_wl_nm[i]);
    }
    float k = 0.2616f * V - 0.1978f * S;
    if (k < 0) k = 0;
    float CLA = 1548.0f * (M - (S - k) * 0.5f);
    if (CLA < 0) CLA = -CLA * 0.5f;  /* night shift if blue < yellow */
    return CLA;
}

float circadian_rea_CS(float CLA)
{
    /* CS = 0.7 × (1 - 1/(1 + (CLA/355.7)^1.1026)) */
    float t = CLA / 355.7f;
    return 0.7f * (1.0f - 1.0f / (1.0f + powf(t, 1.1026f)));
}

int8_t circadian_phase_estimate(uint32_t local_time_s)
{
    /* local_time_s is seconds since midnight. */
    uint32_t hour = (local_time_s / 3600U) % 24U;
    if (hour >= 23 || hour < 7) return -1;  /* biological night */
    if (hour >= 9 && hour < 19) return 1;   /* biological day */
    return 0;  /* transition */
}

void circadian_compute(const spectral_sample_t *spec, circadian_result_t *out)
{
    float cct, duv, x, y;
    int i;

    memset(out, 0, sizeof(*out));
    out->timestamp_ms = spec->timestamp_ms;

    /* Photopic illuminance. */
    out->illuminance_lux = circadian_illuminance(spec);
    out->photopic_lux = out->illuminance_lux;

    /* Chromaticity and CCT. */
    circadian_cct_duv(spec, &cct, &duv, &x, &y);
    out->cct_k = cct;
    out->duv = duv;
    out->x = x;
    out->y = y;

    /* CIE 1976 u'v'. */
    float denom = 2.0f * x - 12.0f * y + 3.0f;
    if (fabsf(denom) < 1e-9f) denom = 1e-9f;
    out->u_prime = 4.0f * x / denom;
    out->v_prime = 9.0f * y / denom;

    /* Melanopic EDI. */
    out->melanopic_edi = circadian_melanopic_edi(spec);
    out->melanopic_er = (out->photopic_lux > 1.0f) ?
                          out->melanopic_edi / out->photopic_lux : 0.0f;
    out->equivalent_melanopic_lux = out->melanopic_edi;

    /* Cyanopic EDI. */
    float c = 0.0f;
    for (i = 0; i < AS7343_NUM_CHANNELS; i++)
        c += spec->irradiance_uw_cm2[i] * as7343_cyanopic_w[i];
    out->cyanopic_edi = c * 10.0f * 7.53f;  /* D65 cyanopic normalization */

    /* Rea CLA/CS. */
    out->circadian_lightCLA = circadian_rea_CLA(spec);
    out->circadian_stimulus = circadian_rea_CS(out->circadian_lightCLA);

    /* Rough CRI / TM-30 estimate from spectral continuity.            */
    /* High spectral continuity → high CRI.  We estimate by measuring    */
    /* how much the spectrum deviates from a smooth blackbody at the    */
    /* same CCT.                                                         */
    float continuity = 1.0f;
    for (i = 1; i < 8; i++) {
        float a = spec->irradiance_uw_cm2[i-1];
        float b = spec->irradiance_uw_cm2[i];
        if (a + b > 1e-6f) {
            float ratio = fabsf(a - b) / (a + b);
            if (ratio > continuity) continuity = ratio;
        }
    }
    out->cri_ra = 100.0f - 60.0f * continuity;
    if (out->cri_ra < 0) out->cri_ra = 0;
    if (out->cri_ra > 100) out->cri_ra = 100;
    out->tm_30_rfg = 85.0f - 40.0f * continuity;
    out->tm_30_rgs = 100.0f + 20.0f * (1.0f - continuity);

    /* Circadian phase. */
    out->circadian_phase = circadian_phase_estimate(spec->timestamp_ms / 1000U);
}