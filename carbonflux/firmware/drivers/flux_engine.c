/**
 * @file    flux_engine.c
 * @brief   Linear regression flux computation for closed-chamber CO₂ accumulation.
 * @author  jayis1
 * @copyright Copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Implements the core flux computation: linear least-squares regression on
 * CO₂ concentration vs. time data from a closed-chamber accumulation.
 * Computes dC/dt (the slope of CO₂ accumulation) and converts to
 * soil CO₂ efflux in µmol·m⁻²·s⁻¹ using chamber geometry and environmental
 * correction (temperature, pressure).
 *
 * This is the critical algorithm that determines measurement quality.
 * Floating-point precision is used throughout (the STM32U5 has an FPU).
 */

#include "flux_engine.h"
#include "../board.h"
#include <math.h>

/* ======================================================================== */
/*  LOCAL STATE                                                             */
/* ======================================================================== */

static struct {
    flux_result_t result;           /* Current accumulation result        */
    float         history[MAX_CO2_SAMPLES]; /* CO₂ samples for R² calc  */
    float         history_t[MAX_CO2_SAMPLES]; /* Time samples            */
    uint32_t      count;            /* Number of valid samples            */
    float         pressure_hpa;     /* Pressure at midpoint (hPa)         */
    float         temp_c;           /* Temperature at midpoint (°C)       */
    bool          accum_active;     /* TRUE if accumulation in progress   */
} g_fe = {0};

/* ======================================================================== */
/*  PUBLIC API                                                              */
/* ======================================================================== */

/**
 * @brief Reset the flux engine for a new accumulation cycle.
 *
 * Must be called before starting each accumulation. Clears all running sums,
 * sample buffers, and counters.
 */
void flux_engine_reset(void) {
    g_fe.count = 0;
    g_fe.accum_active = true;
    g_fe.pressure_hpa = 1013.25f;   /* Default sea-level pressure */
    g_fe.temp_c = 25.0f;             /* Default air temperature */
    g_fe.result.sample_count = 0;
    g_fe.result.sum_x = 0.0f;
    g_fe.result.sum_y = 0.0f;
    g_fe.result.sum_xy = 0.0f;
    g_fe.result.sum_xx = 0.0f;
    g_fe.result.intercept = 0.0f;
    g_fe.result.slope = 0.0f;
    g_fe.result.r_squared = 0.0f;
    g_fe.result.flux_umol = 0.0f;
}

/**
 * @brief Add a single CO₂ sample to the regression.
 *
 * Called every CO2_SAMPLE_INTERVAL_S (2 seconds) during accumulation.
 * Updates running sums in O(1) time. Stores samples for R² computation.
 *
 * @param time_sec   Time since lid closure (seconds, float)
 * @param co2_ppm    CO₂ concentration (ppm, float)
 * @param pressure   Barometric pressure (hPa) — sampled at midpoint
 * @param temp_c     Air temperature in chamber (°C)
 */
void flux_engine_add_sample(float time_sec, float co2_ppm,
                            float pressure, float temp_c) {
    if (!g_fe.accum_active) return;
    if (g_fe.count >= MAX_CO2_SAMPLES) return;

    /* Store for R² computation */
    g_fe.history_t[g_fe.count] = time_sec;
    g_fe.history[g_fe.count] = co2_ppm;
    g_fe.count++;

    /* Update running sums */
    g_fe.result.sum_x += time_sec;
    g_fe.result.sum_y += co2_ppm;
    g_fe.result.sum_xy += time_sec * co2_ppm;
    g_fe.result.sum_xx += time_sec * time_sec;

    /* Update environmental parameters (midpoint sampling) */
    g_fe.pressure_hpa = pressure;
    g_fe.temp_c = temp_c;

    g_fe.result.sample_count = g_fe.count;
}

/**
 * @brief Compute the final flux from accumulated samples.
 *
 * After the accumulation timer expires, this function runs the linear
 * regression and computes soil CO₂ efflux. The result is stored in
 * the provided flux_result_t structure.
 *
 * Flux equation:
 *   F = (dC/dt) × (V × P) / (A × R × T)
 *
 * Where:
 *   F  = flux (µmol·m⁻²·s⁻¹)
 *   dC/dt = CO₂ accumulation rate (ppm/s)
 *   V  = chamber volume (m³)
 *   P  = barometric pressure (kPa)
 *   A  = soil surface area enclosed (m²)
 *   R  = ideal gas constant (8.314462 L·kPa·mol⁻¹·K⁻¹)
 *   T  = air temperature in chamber (K)
 *
 * @param result Output structure with slope, R², and computed flux
 */
void flux_engine_compute(flux_result_t *result) {
    float n = (float)g_fe.count;

    if (n < (float)MIN_VALID_SAMPLES) {
        /* Not enough samples — return zero flux */
        g_fe.result.slope = 0.0f;
        g_fe.result.r_squared = 0.0f;
        g_fe.result.flux_umol = 0.0f;
        if (result) *result = g_fe.result;
        g_fe.accum_active = false;
        return;
    }

    /* Compute linear regression slope (dC/dt) */
    float denom = n * g_fe.result.sum_xx - g_fe.result.sum_x * g_fe.result.sum_x;

    if (fabsf(denom) < 1e-12f) {
        /* Degenerate case — all x values the same */
        g_fe.result.slope = 0.0f;
        g_fe.result.r_squared = 0.0f;
        g_fe.result.flux_umol = 0.0f;
        if (result) *result = g_fe.result;
        g_fe.accum_active = false;
        return;
    }

    g_fe.result.slope = (n * g_fe.result.sum_xy
                       - g_fe.result.sum_x * g_fe.result.sum_y) / denom;
    g_fe.result.intercept = (g_fe.result.sum_y
                           - g_fe.result.slope * g_fe.result.sum_x) / n;

    /* Compute R² (coefficient of determination) */
    float mean_y = g_fe.result.sum_y / n;
    float ss_res = 0.0f;  /* Sum of squared residuals */
    float ss_tot = 0.0f;  /* Total sum of squares */

    for (uint32_t i = 0; i < g_fe.count; i++) {
        float y_pred = g_fe.result.intercept + g_fe.result.slope * g_fe.history_t[i];
        float residual = g_fe.history[i] - y_pred;
        ss_res += residual * residual;
        float diff = g_fe.history[i] - mean_y;
        ss_tot += diff * diff;
    }

    g_fe.result.r_squared = (ss_tot > 0.0f) ? (1.0f - ss_res / ss_tot) : 0.0f;

    /* If R² < 0 (model worse than mean), clamp to 0 */
    if (g_fe.result.r_squared < 0.0f) {
        g_fe.result.r_squared = 0.0f;
    }

    /* Compute flux: F = (dC/dt × V × P) / (A × R × T) */
    float P = g_fe.pressure_hpa / 10.0f;  /* Convert hPa to kPa */
    float T = g_fe.temp_c + 273.15f;       /* Convert °C to Kelvin */
    float V_L = CHAMBER_VOLUME_L;           /* Chamber volume in liters */
    float A = CHAMBER_AREA_M2;              /* Soil surface area in m² */
    float R = 8.314462f;                    /* Gas constant L·kPa/(mol·K) */

    /*
     * Derivation:
     *   dC/dt     = ppm/s = µmol·mol⁻¹·s⁻¹
     *   n = P·V / (R·T)  (moles of air in chamber)
     *   dn_CO2/dt = (dC/dt) × n = (dC/dt) × P·V / (R·T)
     *   flux = (dn_CO2/dt) / A
     *
     * Units check:
     *   dC/dt (ppm/s = µmol/mol·s⁻¹)
     *   × P (kPa) × V (L)
     *   / (R (L·kPa/(mol·K)) × T (K))
     *   / A (m²)
     *   = µmol·m⁻²·s⁻¹ ✓
     */

    /* Only compute flux if slope is positive (CO₂ accumulating) */
    if (g_fe.result.slope > 0.0f) {
        g_fe.result.flux_umol = (g_fe.result.slope * P * V_L)
                              / (A * R * T);
    } else {
        /* Negative or zero slope — no net efflux */
        g_fe.result.flux_umol = 0.0f;
    }

    g_fe.accum_active = false;

    if (result) {
        *result = g_fe.result;
    }
}

/**
 * @brief Get the current number of accumulated samples.
 */
uint32_t flux_engine_get_count(void) {
    return g_fe.count;
}

/**
 * @brief Check if accumulation is in progress.
 */
bool flux_engine_is_active(void) {
    return g_fe.accum_active;
}

/**
 * @brief Compute a quality score for the flux measurement (0–100).
 *
 * Factors:
 *   - R² weight: 40%  (how linear is the CO₂ rise)
 *   - Sample count: 30%  (more samples = more confidence)
 *   - Slope magnitude: 20%  (very low fluxes are harder to measure)
 *   - Temperature stability: 10%  (low drift is better)
 *
 * @return Quality score 0–100
 */
uint8_t flux_engine_quality_score(void) {
    float score = 0.0f;

    /* R² contribution (40% of total) */
    float r2_score = (g_fe.result.r_squared - 0.90f) / 0.10f;
    if (r2_score > 1.0f) r2_score = 1.0f;
    if (r2_score < 0.0f) r2_score = 0.0f;
    score += 40.0f * r2_score;

    /* Sample count contribution (30% of total) */
    float count_score = (float)(g_fe.count - MIN_VALID_SAMPLES)
                      / (float)(MAX_CO2_SAMPLES - MIN_VALID_SAMPLES);
    if (count_score > 1.0f) count_score = 1.0f;
    if (count_score < 0.0f) count_score = 0.0f;
    score += 30.0f * count_score;

    /* Slope contribution (20% of total) — fluxes > 1 µmol·m⁻²·s⁻¹ are easy */
    if (g_fe.result.flux_umol > 1.0f) {
        score += 20.0f;
    } else if (g_fe.result.flux_umol > 0.1f) {
        score += 10.0f;
    }

    /* Temperature stability (10% — ideal conditions get 10%) */
    /* Use R² as a proxy for measurement stability */
    score += 10.0f * r2_score;

    return (uint8_t)(score > 100.0f ? 100.0f : score);
}