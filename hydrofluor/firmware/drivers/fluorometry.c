/*
 * fluorometry.c — HydroFluor PLS unmixing, temp comp & reference normalization
 * Author: jayis1
 * License: MIT
 *
 * Converts the raw 6×4 fluorescence matrix from the photodiode acquisition
 * into calibrated analyte concentrations using an on-device Partial Least
 * Squares (PLS-1) model. Includes:
 *   - Reference photodiode normalization (ratiometric against LED output)
 *   - Quadratic temperature compensation of fluorescence quantum yield
 *   - Dark subtraction already done at acquisition time
 *   - Overrange / bubble flag propagation
 *
 * The PLS model coefficients are stored in QSPI flash and loaded into RAM
 * at boot. Each analyte has its own small model (6 latent variables × 24
 * features). Predictions are computed in fixed-point Q15 arithmetic to
 * avoid pulling in a soft-float library on the Cortex-M4 (though the
 * STM32L4 has an FPU, the PLS path is kept integer for determinism).
 */
#include "fluorometry.h"
#include "led_excitation.h"
#include "../registers.h"
#include <string.h>
#include <stdio.h>

/* Default PLS models — these would normally come from QSPI calibration
 * storage. The values below are *illustrative* coefficients for a synthetic
 * Gaussian-shaped spectral response; the real device is calibrated against
 * NIST-traceable quinine-sulfate (CDOM), chlorophyll-a extract, phycocyanin
 * extract, crude-oil dispersion, and Formazin turbidity standards. The
 * on-device storage layer (storage.c) writes calibrated values over these. */
static pls_model_t g_models[ANALYTE_COUNT];

/* Temperature compensation: conc_corrected = conc_raw * (1 + k1*T + k2*T²)
 * with T in °C. Defaults below are approximate literature values. */
static struct { float k1, k2; } g_tempcomp[ANALYTE_COUNT] = {
    /* CDOM:   ~ −0.02 /°C  (decreases with T) */
    { -0.020f, 0.0003f },
    /* Chl-a:  ~ −0.015 /°C */
    { -0.015f, 0.0002f },
    /* Phycocyanin: ~ −0.01 /°C */
    { -0.010f, 0.0001f },
    /* Oil:    ~ −0.005 /°C (weak dependence) */
    { -0.005f, 0.0000f },
    /* Turbidity (scatter, geometry-limited): ~0 */
    {  0.000f, 0.0000f },
};

/* ---- Q15 helpers ---- */
#define Q15_ONE   32768
static inline int32_t q15_mul(int16_t a, int16_t b) {
    return ((int32_t)a * (int32_t)b) >> 15;
}
static inline int16_t q15_from_float(float f) {
    return (int16_t)(f * Q15_ONE);
}

/* Feature vector flattening: 6 excitations × 4 detectors = 24 features,
 * ordered [ex][det] where det is one of the 4 fluorescence PDs. The two
 * reference channels (PD_REF_255, PD_REF_660) are used for normalization
 * and are NOT part of the 24-feature PLS input. */
static void build_feature_vector(const acquisition_t *acq, int16_t feat[PLS_FEATURES])
{
    /* Normalize each fluorescence channel by the appropriate reference.
     * For 255nm-excited oil signal, normalize by ref_255.
     * For 660nm-excited scatter, normalize by ref_660.
     * For the rest, no direct reference exists — we use a fixed scaling. */
    int32_t r255 = acq->ref_255_uv ? acq->ref_255_uv : 1;
    int32_t r660 = acq->ref_660_uv ? acq->ref_660_uv : 1;

    int idx = 0;
    for (int ex = 0; ex < EX_CHANNEL_COUNT; ++ex) {
        for (int det = 0; det < 4; ++det) {   /* only fluorescence PDs 0-3 */
            int32_t v = acq->fluor[ex][det];
            /* Ratiometric normalization where a reference exists */
            if (ex == 0 && det == 0) v = (v * 1000) / r255;  /* oil/ref255 */
            if (ex == 5 && det == 0) v = (v * 1000) / r660;  /* turb/ref660 */
            /* Scale to int16 range (assume max ~32767000 nV → /1000) */
            v = v / 1000;
            if (v >  32767) v =  32767;
            if (v < -32768) v = -32768;
            feat[idx++] = (int16_t)v;
        }
    }
}

/* Run one PLS-1 model: returns predicted concentration (in analyte units). */
static int32_t pls_predict(const pls_model_t *m, const int16_t feat[PLS_FEATURES])
{
    int32_t t[PLS_LATENT_VARS];
    for (int i = 0; i < PLS_LATENT_VARS; ++i) {
        int32_t acc = 0;
        for (int j = 0; j < PLS_FEATURES; ++j) {
            /* (feat - mu_x) * w  in Q15 */
            int16_t dc = feat[j] - m->mu_x[j];
            acc += q15_mul(dc, m->w[i][j]);
        }
        t[i] = acc + m->b[i];
    }
    int32_t y = m->mu_y;
    for (int i = 0; i < PLS_LATENT_VARS; ++i) {
        y += q15_mul((int16_t)t[i], m->beta[i]);
    }
    return y;   /* in Q15 analyte units */
}

int fluorometry_load_models(void)
{
    /* In a full implementation, read from QSPI flash via the storage driver.
     * Here we initialize with synthetic default models so the firmware
     * runs stand-alone for bring-up. Each model is a near-identity mapping
     * that picks out the primary channel for its analyte. */
    for (int a = 0; a < ANALYTE_COUNT; ++a) {
        pls_model_t *m = &g_models[a];
        memset(m, 0, sizeof(*m));
        /* Weight matrix: w[i][j] — set the dominant channel per analyte. */
        /* Feature index for (ex, det=0) primary detector = ex*4. */
        int primary_ex;
        switch (a) {
            case ANALYTE_CDOM: primary_ex = 2; break;  /* 365→430 */
            case ANALYTE_CHLA: primary_ex = 3; break;  /* 470→685 */
            case ANALYTE_PHYC: primary_ex = 4; break;  /* 590→650 */
            case ANALYTE_OIL:  primary_ex = 0; break;  /* 255→360 */
            case ANALYTE_TURB: primary_ex = 5; break;  /* 660 scatter */
            default: primary_ex = 0;
        }
        int primary_idx = primary_ex * 4;  /* det 0 */
        m->w[0][primary_idx] = Q15_ONE;     /* identity */
        m->beta[0]           = Q15_ONE;
        m->mu_y              = 0;
        m->b[0]              = 0;
        m->scale             = 1;
        /* Latent vars 1-5 unused (zero). */
    }
    return 0;
}

void fluorometry_set_temp_comp(analyte_t a, float k1, float k2)
{
    if (a >= ANALYTE_COUNT) return;
    g_tempcomp[a].k1 = k1;
    g_tempcomp[a].k2 = k2;
}

void fluorometry_unmix(const acquisition_t *acq,
                        int16_t temp_c01,
                        analyte_result_t *out)
{
    int16_t feat[PLS_FEATURES];
    build_feature_vector(acq, feat);

    /* Temperature factor: 1 + k1*T + k2*T²  (T in °C) */
    float T = temp_c01 / 100.0f;

    int32_t cdom = pls_predict(&g_models[ANALYTE_CDOM], feat);
    int32_t chla = pls_predict(&g_models[ANALYTE_CHLA], feat);
    int32_t phyc = pls_predict(&g_models[ANALYTE_PHYC], feat);
    int32_t oil  = pls_predict(&g_models[ANALYTE_OIL],  feat);
    int32_t turb = pls_predict(&g_models[ANALYTE_TURB], feat);

    /* Apply temperature compensation (in float — acceptable on M4 FPU) */
    for (int a = 0; a < ANALYTE_COUNT; ++a) {
        float factor = 1.0f
                     + g_tempcomp[a].k1 * T
                     + g_tempcomp[a].k2 * T * T;
        int32_t *val[5] = { &cdom, &chla, &phyc, &oil, &turb };
        float corrected = (float)*val[a] * factor;
        if (corrected < 0) corrected = 0;
        *val[a] = (int32_t)corrected;
    }

    /* Convert from Q15 to engineering units (scale 1 → ppb/μg/L/NTU) */
    out->cdom_ppb     = (uint16_t)(cdom / 100);
    out->chla_ugl     = (uint16_t)(chla / 100);
    out->phyc_ugl     = (uint16_t)(phyc / 100);
    out->oil_ppb      = (uint16_t)(oil  / 100);
    out->turb_ntu_x100 = (uint16_t)(turb / 10);  /* 0.01 NTU resolution */
    out->flags        = acq->flags;
}

int fluorometry_calibrate(analyte_t which, uint16_t ref_value,
                            const int32_t raw[PLS_FEATURES])
{
    if (which >= ANALYTE_COUNT) return -1;
    /* Two-point calibration: we adjust the model's mu_y (offset) and beta[0]
     * (slope) so that the predicted concentration matches ref_value for the
     * supplied raw feature vector. This is a simplified single-point
     * adjustment; a full calibration collects several points and refits. */
    int16_t feat[PLS_FEATURES];
    for (int i = 0; i < PLS_FEATURES; ++i) {
        int32_t v = raw[i] / 1000;
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        feat[i] = (int16_t)v;
    }
    int32_t pred = pls_predict(&g_models[which], feat);
    int32_t target = (int32_t)ref_value * 100;   /* to Q15 */
    /* Adjust offset so that pred + offset == target */
    int16_t delta = (int16_t)((target - pred) & 0xFFFF);
    g_models[which].mu_y += delta;
    return 0;
}

void fluorometry_format(const analyte_result_t *r, char *buf, size_t len)
{
    snprintf(buf, len,
        "CDOM=%u ppb  Chl-a=%u ug/L  Phyc=%u ug/L  Oil=%u ppb  Turb=%u.%02u NTU  flags=%u",
        r->cdom_ppb, r->chla_ugl, r->phyc_ugl, r->oil_ppb,
        r->turb_ntu_x100 / 100, r->turb_ntu_x100 % 100, r->flags);
}