/*
 * classifier.c — PCA projection + Mahalanobis distance + mixture interp.
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The classifier has three stages:
 *
 *  1. feature build — concatenates the 8 optical absorbances with the
 *     40 real/imag EIS components and the sample temperature into a
 *     49-dim raw vector.
 *  2. PCA projection — multiplies the mean-centred raw vector by a
 *     fixed 16×49 whitening matrix learned offline (from a calibration
 *     corpus) and stored as Q16.16 fixed-point in flash. The output is
 *     a 16-dim PCA-space point z. We keep the maths in fixed-point so
 *     no per-measurement heap or float matrix multiply is needed.
 *  3. classification — computes the squared Mahalanobis distance from
 *     z to each class centroid (using the class's diagonal covariance),
 *     converts distances to a softmax probability, picks the top class,
 *     and — if the two nearest classes are a known adulterant pair —
 *     interpolates the fractional position along the line between the
 *     centroids to estimate the adulteration ratio.
 */
#include "classifier.h"
#include <string.h>
#include <math.h>

/* ---- Q16.16 helpers ------------------------------------------------- */
int32_t q16_from_float(float x) {
    if (x > 32767.0f) x = 32767.0f;
    if (x < -32768.0f) x = -32768.0f;
    return (int32_t)(x * 65536.0f);
}
float q16_to_float(int32_t q) { return (float)q / 65536.0f; }

/* ---- PCA / whitening matrix ---------------------------------------- */
/* Mean vector (49-dim) and whitening matrix (16×49) learned offline
 * from the calibration corpus. We embed a representative slice here;
 * a production build loads these from a QSPI blob alongside the
 * library. Values are Q16.16 fixed point.
 *
 * Only the first 8 rows × 8 columns are shown in full; the rest are
 * zero so the fixed-point multiply still completes quickly. */
static const int32_t pca_mean[FEATURE_DIM_RAW] = {
    /* optical absorbances are usually small; EIS real/imag larger  */
    0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, 0x10000, /* 8 optical  */
    0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
    0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
    0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
    0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000,
    0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, 0x20000, /* 40 EIS */
    0x00A00000                                              /* temp ~25°C Q16 */
};

/* A 16×49 whitening matrix. For brevity we keep only the first 4 rows
 * non-trivial; the firmware still produces a 16-dim projection, with
 * the trailing 12 dimensions identically zero (which is fine for the
 * Mahalanobis classifier — those components just don't contribute). */
static const int32_t pca_W[FEATURE_DIM_PCA][FEATURE_DIM_RAW] = {
    { 0x10000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
      0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000,
      0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000,
      0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000,
      0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000,
      0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000, 0x04000,
      0x00100 },
    { 0x00000, 0x10000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
      0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000,
      0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000,
      0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000,
      0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000,
      0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000, 0x02000,
      0x00080 },
    { 0x00000, 0x00000, 0x10000, 0x00000, 0x00000, 0x00000, 0x00000, 0x00000,
      0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000,
      0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000,
      0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000,
      0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000,
      0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000, 0x01000,
      0x00040 },
    { 0x00000, 0x00000, 0x00000, 0x10000, 0x00000, 0x00000, 0x00000, 0x00000,
      0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800,
      0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800,
      0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800,
      0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800,
      0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800, 0x00800,
      0x00020 },
    /* rows 4..15 all-zero — kept empty for flash density */
};

/* ---- Adulterant pair table (app-configurable via BLE) ------------- */
typedef struct { uint16_t a; uint16_t b; float threshold; } adulterant_pair_t;
static adulterant_pair_t pairs[8] = {
    { 1,  0, 0.10f },   /* milk / water  */
    { 4,  0, 0.05f },   /* whisky / water */
    { 9, 10, 0.10f },   /* honey / glucose syrup */
    { 11, 12, 0.08f },  /* petrol / kerosene */
    { 13, 14, 0.05f },  /* distilled / saline */
    { 0,  0, 0.0f },    { 0, 0, 0.0f }, { 0, 0, 0.0f }
};
static const uint8_t n_pairs = 5;

/* ---- Feature build ------------------------------------------------- */
void classifier_build_feature(float raw[FEATURE_DIM_RAW],
                              const float opt[OPTICAL_WAVELENGHS],
                              const eis_point_t eis[EIS_FREQ_POINTS],
                              float temp_c)
{
    uint8_t k = 0;
    for (uint8_t i = 0; i < OPTICAL_WAVELENGHS; ++i) raw[k++] = opt[i];
    for (uint8_t i = 0; i < EIS_FREQ_POINTS; ++i) {
        raw[k++] = eis[i].re;
        raw[k++] = eis[i].im;
    }
    /* Normalise conductivity to 25 °C (≈2 %/°C). */
    float tnorm = 1.0f / (1.0f + 0.02f * (temp_c - 25.0f));
    for (uint8_t i = OPTICAL_WAVELENGHS; i < OPTICAL_WAVELENGHS + 2 * EIS_FREQ_POINTS; ++i)
        raw[i] *= tnorm;
    raw[k] = temp_c;
}

/* ---- PCA projection (Q16.16 fixed point) -------------------------- */
void classifier_project(const float raw[FEATURE_DIM_RAW],
                        int32_t z[FEATURE_DIM_PCA])
{
    /* Convert raw → Q16.16 and mean-centre. */
    int32_t xr[FEATURE_DIM_RAW];
    for (uint8_t i = 0; i < FEATURE_DIM_RAW; ++i)
        xr[i] = q16_from_float(raw[i]) - pca_mean[i];

    /* z = W · (x − mean), using 64-bit accumulator. */
    for (uint8_t r = 0; r < FEATURE_DIM_PCA; ++r) {
        int64_t acc = 0;
        for (uint8_t c = 0; c < FEATURE_DIM_RAW; ++c)
            acc += (int64_t)xr[c] * (int64_t)pca_W[r][c];
        z[r] = (int32_t)(acc >> 16);   /* back to Q16.16               */
    }
}

/* ---- Mahalanobis distance + softmax + mixture interp -------------- */
static int32_t mahalanobis_sq(const liquid_class_t *cls,
                              const int32_t z[FEATURE_DIM_PCA])
{
    /* Diagonal covariance; D² = Σ (z−μ)² / σ²  (all Q16.16). */
    int64_t acc = 0;
    for (uint8_t i = 0; i < FEATURE_DIM_PCA; ++i) {
        int32_t d = z[i] - cls->centroid[i];
        int32_t v = cls->variance[i];
        if (v < 0x100) v = 0x100;             /* floor variance          */
        /* (d*d)/v in Q16.16 = (d²/65536)/(v/65536) × 65536 = d²/v       */
        acc += (int64_t)d * d / (int64_t)v;
    }
    if (acc > 0x7FFFFFFFLL) acc = 0x7FFFFFFFLL;
    return (int32_t)acc;
}

hydra_err_t classifier_classify(const library_t *lib,
                                const int32_t z[FEATURE_DIM_PCA],
                                classify_result_t *out)
{
    if (!lib || !out) return HYDRA_ERR_IO;
    if (lib->n_classes == 0) return HYDRA_ERR_CALIB;

    /* 1. squared distances → softmax probabilities. */
    int32_t d2[MAX_LIQUID_CLASSES];
    float   p[MAX_LIQUID_CLASSES];
    float   sum = 0.0f;
    for (uint32_t i = 0; i < lib->n_classes; ++i) {
        d2[i] = mahalanobis_sq(&lib->classes[i], z);
        p[i]  = expf(-0.5f * q16_to_float(d2[i]));
        sum  += p[i];
    }
    if (sum <= 0.0f) return HYDRA_ERR_NOCLASS;

    /* 2. top class. */
    uint32_t best = 0;
    for (uint32_t i = 1; i < lib->n_classes; ++i)
        if (p[i] > p[best]) best = i;

    out->class_id    = lib->classes[best].class_id;
    out->confidence  = p[best] / sum;
    memcpy(out->name, lib->classes[best].name, NAME_LEN);
    out->adulterant = 0;
    out->adulterant_ratio = 0.0f;
    memcpy(out->pca_z, z, sizeof(out->pca_z));

    /* 3. second-best class. */
    uint32_t second = (best == 0) ? 1 : 0;
    for (uint32_t i = 0; i < lib->n_classes; ++i)
        if (i != best && p[i] > p[second]) second = i;

    /* 4. adulterant-pair check + interpolation along the centroid line. */
    for (uint8_t i = 0; i < n_pairs; ++i) {
        uint16_t a = pairs[i].a, b = pairs[i].b;
        if ((lib->classes[best].class_id == a && lib->classes[second].class_id == b) ||
            (lib->classes[best].class_id == b && lib->classes[second].class_id == a)) {

            /* Find the two class indices for a, b. */
            uint32_t ia = 0, ib = 0;
            for (uint32_t k = 0; k < lib->n_classes; ++k) {
                if (lib->classes[k].class_id == a) ia = k;
                if (lib->classes[k].class_id == b) ib = k;
            }
            /* Project z onto the segment μ_a .. μ_b. */
            int64_t num = 0, den = 0;
            for (uint8_t d = 0; d < FEATURE_DIM_PCA; ++d) {
                int32_t u = lib->classes[ia].centroid[d];
                int32_t v = lib->classes[ib].centroid[d];
                int32_t w = v - u;
                num += (int64_t)(z[d] - u) * w;
                den += (int64_t)w * w;
            }
            float t = (den > 0) ? (float)((double)num / (double)den) : 0.0f;
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            /* Raise the flag if the interpolated fraction exceeds the
             * stored threshold (i.e. the sample is meaningfully off
             * the pure centroid toward the adulterant). */
            float frac = (lib->classes[best].class_id == a) ? t : (1.0f - t);
            if (frac > pairs[i].threshold) {
                out->adulterant = 1;
                out->adulterant_ratio = frac;
            }
            break;
        }
    }
    return HYDRA_OK;
}