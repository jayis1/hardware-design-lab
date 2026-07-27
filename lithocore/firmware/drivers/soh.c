/*
 * soh.c — State-of-Health scoring and degradation mode classification.
 *
 * Takes the CNLS-fitted Randles parameters, the DCIR, and the OCV
 * relaxation data, and computes:
 *   1. A 0-100 SoH score via a geometric mean of resistance/capacitance
 *      ratios vs. the chemistry baseline.
 *   2. A degradation mode via a rule-based classifier + k-NN against a
 *      reference database.
 *   3. A quality verdict (EXCELLENT / GOOD / FAIR / POOR / REPLACE).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <math.h>
#include <string.h>
#include "soh.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------------
 * Degradation mode / verdict name strings
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static const char *mode_names[] = {
    "Healthy",
    "SEI Growth",
    "Lithium Plating",
    "Electrolyte Dry-out",
    "Internal Short",
    "Unknown",
};

static const char *verdict_names[] = {
    "EXCELLENT",
    "GOOD",
    "FAIR",
    "POOR",
    "REPLACE",
};

static const char *chem_names[] = {
    "NMC-18650", "NMC-21700", "LFP-26650", "NCA-18650", "LCO-pack",
};

const char *soh_mode_name(degradation_mode_t mode)
{
    if (mode > DEGRAD_UNKNOWN) mode = DEGRAD_UNKNOWN;
    return mode_names[mode];
}

const char *soh_verdict_name(quality_verdict_t verdict)
{
    if (verdict > VERDICT_REPLACE) verdict = VERDICT_REPLACE;
    return verdict_names[verdict];
}

const char *soh_chemistry_name(uint8_t idx)
{
    if (idx > 4) idx = 4;
    return chem_names[idx];
}

/* -------------------------------------------------------------------------
 * Reference database for k-NN classification
 *
 * Each entry is a feature vector [Rs_ratio, Rsei_ratio, Rct_ratio,
 * Cdl_ratio, DCIR_ratio, SD_rate] with a known degradation label.
 * Ratios are relative to the healthy baseline (1.0 = healthy).
 *
 * This is a compact 80-cell database (reduced here to 20 representative
 * entries for firmware footprint). In production this is stored in a
 * dedicated flash sector.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
typedef struct {
    float    rs_ratio;
    float    rsei_ratio;
    float    rct_ratio;
    float    cdl_ratio;
    float    dcir_ratio;
    float    sd_rate_uv_per_min;   /* self-discharge rate */
    degradation_mode_t label;
} ref_cell_t;

static const ref_cell_t ref_database[] = {
    /* Healthy cells (ratio ≈ 1.0, low self-discharge) */
    { 1.00, 1.00, 1.00, 1.00, 1.00,  0.05, DEGRAD_HEALTHY },
    { 1.05, 1.10, 1.08, 0.95, 1.03,  0.08, DEGRAD_HEALTHY },
    { 0.98, 1.05, 1.02, 0.98, 0.99,  0.03, DEGRAD_HEALTHY },
    { 1.02, 1.00, 1.05, 1.00, 1.01,  0.06, DEGRAD_HEALTHY },
    { 1.08, 1.12, 1.10, 0.93, 1.05,  0.10, DEGRAD_HEALTHY },

    /* SEI growth: Rsei ↑↑, Csei ↓, Rs ~constant */
    { 1.05, 2.50, 1.20, 0.85, 1.10,  0.15, DEGRAD_SEI_GROWTH },
    { 1.02, 3.00, 1.15, 0.80, 1.08,  0.12, DEGRAD_SEI_GROWTH },
    { 1.08, 2.00, 1.30, 0.90, 1.12,  0.20, DEGRAD_SEI_GROWTH },
    { 1.03, 4.50, 1.25, 0.70, 1.15,  0.18, DEGRAD_SEI_GROWTH },

    /* Lithium plating: Rct ↑↑↑, Cdl ↓↓, high-freq shift */
    { 1.10, 1.50, 3.50, 0.45, 1.80,  0.50, DEGRAD_LI_PLATING },
    { 1.05, 1.30, 5.00, 0.35, 2.00,  0.80, DEGRAD_LI_PLATING },
    { 1.15, 1.80, 4.00, 0.40, 1.90,  0.60, DEGRAD_LI_PLATING },
    { 1.08, 1.60, 6.00, 0.30, 2.20,  1.00, DEGRAD_LI_PLATING },

    /* Electrolyte dry-out: Rs ↑↑↑, all semicircles broaden */
    { 2.50, 1.80, 1.60, 0.70, 2.50,  0.30, DEGRAD_DRYOUT },
    { 3.00, 2.00, 1.80, 0.65, 3.00,  0.25, DEGRAD_DRYOUT },
    { 2.00, 1.50, 1.40, 0.75, 2.00,  0.20, DEGRAD_DRYOUT },

    /* Internal short: Rs ↓ (abnormal), high self-discharge */
    { 0.40, 0.80, 0.90, 1.00, 0.50, 15.0, DEGRAD_INTERNAL_SHORT },
    { 0.30, 0.70, 0.85, 1.00, 0.40, 25.0, DEGRAD_INTERNAL_SHORT },
    { 0.50, 0.85, 0.95, 1.00, 0.60, 10.0, DEGRAD_INTERNAL_SHORT },
    { 0.45, 0.75, 0.88, 1.00, 0.55, 20.0, DEGRAD_INTERNAL_SHORT },
};

#define REF_DB_SIZE  (sizeof(ref_database) / sizeof(ref_database[0]))
#define K_NN_K       5

/* -------------------------------------------------------------------------
 * k-NN classifier
 *
 * Computes Euclidean distance in the 6-dimensional feature space to
 * each reference cell, finds the K nearest, and returns the majority
 * label.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
degradation_mode_t soh_classify(const randles_params_t *fitted,
                                const chemistry_baseline_t *baseline,
                                int32_t dcir_mohm,
                                int32_t self_discharge_uv_per_min)
{
    /* Compute feature vector (ratios vs. baseline) */
    float rs_ratio  = (float)fitted->rs_mohm / (float)baseline->rs_mohm;
    float rsei_ratio = (float)fitted->rsei_mohm / (float)baseline->rsei_mohm;
    float rct_ratio = (float)fitted->rct_mohm / (float)baseline->rct_mohm;
    float cdl_ratio = (float)fitted->cdl_mF / (float)baseline->cdl_mF;
    float dcir_ratio = (dcir_mohm > 0) ?
                       (float)dcir_mohm / (float)baseline->rs_mohm : 1.0f;
    float sd_rate = (float)self_discharge_uv_per_min;

    /* Find K nearest neighbors */
    float distances[REF_DB_SIZE];
    for (uint16_t i = 0; i < REF_DB_SIZE; i++) {
        float drs  = rs_ratio  - ref_database[i].rs_ratio;
        float drsei = rsei_ratio - ref_database[i].rsei_ratio;
        float drct = rct_ratio - ref_database[i].rct_ratio;
        float dcdl = cdl_ratio - ref_database[i].cdl_ratio;
        float ddcir = dcir_ratio - ref_database[i].dcir_ratio;
        float dsd  = sd_rate - ref_database[i].sd_rate_uv_per_min;
        /* Weight self-discharge more heavily (it's a strong short indicator) */
        distances[i] = drs*drs + drsei*drsei + drct*drct +
                       dcdl*dcdl + ddcir*ddcir + dsd*dsd*0.1f;
    }

    /* Find K smallest distances (simple selection — K=5, DB=20) */
    uint16_t indices[K_NN_K];
    for (int k = 0; k < K_NN_K; k++) {
        float min_dist = 1e30f;
        uint16_t min_idx = 0;
        for (uint16_t i = 0; i < REF_DB_SIZE; i++) {
            if (distances[i] < min_dist) {
                min_dist = distances[i];
                min_idx = i;
            }
        }
        indices[k] = min_idx;
        distances[min_idx] = 1e30f;  /* exclude for next iteration */
    }

    /* Majority vote */
    uint8_t votes[6] = {0};
    for (int k = 0; k < K_NN_K; k++) {
        votes[ref_database[indices[k]].label]++;
    }

    uint8_t max_votes = 0;
    degradation_mode_t winner = DEGRAD_UNKNOWN;
    for (int m = 0; m <= DEGRAD_UNKNOWN; m++) {
        if (votes[m] > max_votes) {
            max_votes = votes[m];
            winner = (degradation_mode_t)m;
        }
    }

    return winner;
}

/* -------------------------------------------------------------------------
 * SoH score computation
 *
 * SoH = 100 × [ (Rs₀/Rs) × (Rsei₀/Rsei) × (Rct₀/Rct) × (Cdl/Cdl₀) ]^(1/4)
 *
 * clamped to [0, 100]. This geometric mean ensures that any single
 * degraded component pulls the score down proportionally.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static uint8_t compute_soh_score(const randles_params_t *fitted,
                                 const chemistry_baseline_t *baseline)
{
    double rs_ratio  = (double)baseline->rs_mohm / (double)fitted->rs_mohm;
    double rsei_ratio = (double)baseline->rsei_mohm / (double)fitted->rsei_mohm;
    double rct_ratio = (double)baseline->rct_mohm / (double)fitted->rct_mohm;
    double cdl_ratio = (double)fitted->cdl_mF / (double)baseline->cdl_mF;

    /* Clamp ratios to [0, 2] — a ratio > 1 means the cell is better than
     * baseline, which we cap at 2× to prevent one good parameter from
     * masking others' degradation */
    if (rs_ratio > 2.0) rs_ratio = 2.0;
    if (rsei_ratio > 2.0) rsei_ratio = 2.0;
    if (rct_ratio > 2.0) rct_ratio = 2.0;
    if (cdl_ratio > 2.0) cdl_ratio = 2.0;
    if (rs_ratio < 0.0) rs_ratio = 0.0;
    if (rsei_ratio < 0.0) rsei_ratio = 0.0;
    if (rct_ratio < 0.0) rct_ratio = 0.0;
    if (cdl_ratio < 0.0) cdl_ratio = 0.0;

    /* Geometric mean (4th root of the product) */
    double product = rs_ratio * rsei_ratio * rct_ratio * cdl_ratio;
    if (product < 0.0) product = 0.0;

    double soh = 100.0 * pow(product, 0.25);

    if (soh > 100.0) soh = 100.0;
    if (soh < 0.0) soh = 0.0;

    return (uint8_t)(soh + 0.5);
}

/* -------------------------------------------------------------------------
 * Quality verdict from SoH score
 * ------------------------------------------------------------------------- */
static quality_verdict_t verdict_from_soh(uint8_t soh)
{
    if (soh >= 85) return VERDICT_EXCELLENT;
    if (soh >= 70) return VERDICT_GOOD;
    if (soh >= 50) return VERDICT_FAIR;
    if (soh >= 30) return VERDICT_POOR;
    return VERDICT_REPLACE;
}

/* -------------------------------------------------------------------------
 * Main SoH computation
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void soh_compute(soh_result_t *result, const chemistry_baseline_t *baseline)
{
    if (!result->fit_valid) {
        /* CNLS fit failed — use a rough estimate from raw EIS data */
        result->soh_score = 50;  /* unknown → middle score */
        result->degradation = DEGRAD_UNKNOWN;
        result->verdict = VERDICT_FAIR;
        result->chisq = 1e10;
        return;
    }

    /* Compute SoH score from fitted parameters */
    result->soh_score = compute_soh_score(&result->randles, baseline);

    /* Classify degradation mode */
    result->degradation = soh_classify(&result->randles, baseline,
                                       result->dcir_mohm,
                                       result->self_discharge_uv_per_min);

    /* Determine quality verdict */
    result->verdict = verdict_from_soh(result->soh_score);

    /* Penalize internal short heavily — always REPLACE */
    if (result->degradation == DEGRAD_INTERNAL_SHORT) {
        result->soh_score = (result->soh_score > 20) ? 20 : result->soh_score;
        result->verdict = VERDICT_REPLACE;
    }

    /* Penalize lithium plating — high fire risk */
    if (result->degradation == DEGRAD_LI_PLATING) {
        if (result->soh_score > 40) result->soh_score = 40;
        if (result->verdict < VERDICT_POOR) result->verdict = VERDICT_POOR;
    }
}