/*
 * disdrometer.c — DSD computation, derived products, calibration
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * Accumulates per-droplet features into a 9-bin drop-size distribution
 * (DSD) histogram, then computes the standard meteorological derived
 * products:
 *
 *   N(D)  — number concentration (m^-3 mm^-1)
 *   R     — rainfall rate (mm/hr)
 *   Z     — radar reflectivity factor (mm^6 m^-3)
 *   LWC   — liquid water content (g/m^3)
 *   Z-R   — Marshall-Z palmer fit Z = a·R^b
 *   D_m   — volume-weighted mean diameter (mm)
 *   D_0   — median volume diameter (mm)
 *
 * The terminal velocity is computed using the Atlas (1977) empirical
 * formula, valid for 0.1-7 mm droplets at sea level:
 *   v(D) = 9.65 - 10.3·exp(-0.6·D)   [m/s, D in mm]
 *
 * Sampling geometry: the impact plate has an effective catch area of
 * A = π·(0.045)² = 6.36×10^-3 m² (90 mm diameter, minus the central
 * deflector). The volume sampled per unit time is A · v(D) · Δt, so
 *   N(D) = count(D) / (A · v(D) · Δt · ΔD)
 */
#include <math.h>
#include <string.h>
#include "../board.h"
#include "disdrometer.h"

/* ---- DSD bin edges (mm) — Marshall-Palmer scale ---- */
const float DSD_BIN_EDGES[DSD_NUM_BINS + 1] = {
    0.3f, 0.6f, 1.0f, 1.5f, 2.0f, 2.5f, 3.5f, 4.5f, 5.5f, 7.0f
};

/* ---- Default calibration coefficients (rough factory defaults) ----
 * These are derived from a generic PVDF cantilever model and will be
 * overwritten by the per-unit lab calibration stored in FRAM.
 *
 * Diameter model (mm):
 *   D = 0.085·peak(mV) + 0.002·width(µs) + 0.01·decay(µs) + 0.15
 * Velocity model (m/s):
 *   v = 450 / width(µs) + 1.2
 * Charge model (pC):
 *   q = 120·(asymmetry - 0.5) + 0
 * Energy scale:
 *   E_harvested = 0.0008 · Σ(V²dt) in mV²·ms → Joules
 */
static const calibration_t s_default_cal = {
    .diameter_a   = 0.085f,
    .diameter_b   = 0.002f,
    .diameter_c   = 0.01f,
    .diameter_d   = 0.15f,
    .velocity_a   = 450.0f,
    .velocity_b   = 1.2f,
    .charge_a     = 120.0f,
    .charge_b     = 0.0f,
    .energy_scale = 0.0008f,
    .magic        = CALIB_MAGIC
};

const calibration_t *disdrometer_default_cal(void)
{
    return &s_default_cal;
}

/* ================================================================
 * Terminal velocity (Atlas 1977)
 * ================================================================ */
float terminal_velocity(float diameter_mm)
{
    if (diameter_mm <= 0.0f) return 0.0f;
    if (diameter_mm > 7.0f) diameter_mm = 7.0f;
    return 9.65f - 10.3f * expf(-0.6f * diameter_mm);
}

/* ================================================================
 * Find the bin index for a given diameter
 * ================================================================ */
int disdrometer_bin_index(float diameter_mm)
{
    for (int i = 0; i < DSD_NUM_BINS; ++i) {
        if (diameter_mm < DSD_BIN_EDGES[i + 1])
            return i;
    }
    return DSD_NUM_BINS - 1;   /* last bin catches everything >= last edge */
}

/* ================================================================
 * Initialize / reset the DSD accumulator
 * ================================================================ */
void disdrometer_init(dsd_t *dsd)
{
    memset(dsd, 0, sizeof(*dsd));
}

void disdrometer_reset(dsd_t *dsd)
{
    memset(dsd->count, 0, sizeof(dsd->count));
    dsd->pos_count = 0;
    dsd->neg_count = 0;
    dsd->avg_charge_pc = 0;
}

/* ================================================================
 * Add a single droplet to the DSD histogram
 * ================================================================ */
void disdrometer_add(dsd_t *dsd, const droplet_feature_t *feat)
{
    int bin = disdrometer_bin_index(feat->diameter_mm);
    dsd->count[bin]++;

    /* Charge tally */
    if (feat->charge_pc > 0.5f)
        dsd->pos_count++;
    else if (feat->charge_pc < -0.5f)
        dsd->neg_count++;

    /* Running sum for average charge */
    dsd->avg_charge_pc += (int16_t)feat->charge_pc;
}

/* ================================================================
 * Compute derived products from the accumulated histogram
 * ================================================================ */
void disdrometer_compute(dsd_t *dsd, float sample_area_m2, float interval_s)
{
    float rho_water = 1000.0f;   /* kg/m³ */
    float pi = 3.14159265f;

    /* Effective catch area: 90 mm plate minus 10 mm central deflector
     * A = π·((0.045)² - (0.005)²) = 6.28×10^-3 m²
     * But the caller passes the calibrated value. */
    float A = sample_area_m2;
    if (A <= 0) A = 6.28e-3f;

    float total_count = 0;
    float total_lwc = 0;
    float total_z = 0;
    float total_r = 0;
    float volume_sum = 0;   /* for D_m */
    float lwc_cumulative[DSD_NUM_BINS];

    for (int i = 0; i < DSD_NUM_BINS; ++i) {
        float D_center = 0.5f * (DSD_BIN_EDGES[i] + DSD_BIN_EDGES[i + 1]);   /* mm */
        float dD = DSD_BIN_EDGES[i + 1] - DSD_BIN_EDGES[i];                  /* mm */
        float D_m = D_center * 0.001f;   /* → meters */
        float v = terminal_velocity(D_center);                              /* m/s */

        /* N(D) = count / (A · v · Δt · ΔD)  [m^-3 mm^-1] */
        if (v > 0 && interval_s > 0 && dD > 0) {
            dsd->n_per_m3[i] = (float)dsd->count[i] / (A * v * interval_s * dD);
        } else {
            dsd->n_per_m3[i] = 0;
        }

        /* Rainfall rate contribution: R = (π/6)·N(D)·D³·v(D)·dD  [mm/hr]
         * D in mm, v in m/s → convert: m/s × 1000 mm/m × 3600 s/hr */
        float D_mm = D_center;
        float r_contrib = (pi / 6.0f) * dsd->n_per_m3[i]
                        * D_mm * D_mm * D_mm * v * dD * 3600.0f;
        total_r += r_contrib;

        /* Reflectivity: Z = Σ N(D)·D^6·dD  [mm^6/m^3] */
        float z_contrib = dsd->n_per_m3[i] * powf(D_mm, 6.0f) * dD;
        total_z += z_contrib;

        /* Liquid water content: LWC = (π/6)·ρ·Σ N(D)·D³·dD  [g/m³]
         * N(D) in m^-3 mm^-1, D in mm, dD in mm → need consistent units.
         * LWC = (π/6)·ρ_water·Σ N(D)·(D·10^-3)³·(dD·10^-3)  [kg/m³]
         *     × 1000 → g/m³ */
        float lwc_contrib = (pi / 6.0f) * rho_water
                          * dsd->n_per_m3[i]
                          * powf(D_mm * 0.001f, 3.0f)
                          * (dD * 0.001f) * 1000.0f;
        total_lwc += lwc_contrib;
        lwc_cumulative[i] = total_lwc;

        /* Volume-weighted mean diameter: D_m = Σ(N·D⁴·dD) / Σ(N·D³·dD) */
        volume_sum += dsd->n_per_m3[i] * powf(D_mm, 4.0f) * dD;

        total_count += dsd->count[i];
    }

    dsd->rainfall_rate = total_r;
    dsd->reflectivity  = total_z;
    dsd->liquid_water_content = total_lwc;
    dsd->total_droplets = total_count;

    /* Volume-weighted mean diameter */
    float denom = 0;
    for (int i = 0; i < DSD_NUM_BINS; ++i) {
        float D_center = 0.5f * (DSD_BIN_EDGES[i] + DSD_BIN_EDGES[i + 1]);
        float dD = DSD_BIN_EDGES[i + 1] - DSD_BIN_EDGES[i];
        denom += dsd->n_per_m3[i] * powf(D_center, 3.0f) * dD;
    }
    dsd->mean_diameter = (denom > 0) ? (volume_sum / denom) : 0;

    /* Median volume diameter D0: the diameter at which LWC cumulative
     * reaches half of total LWC. Linear interpolation between bins. */
    if (total_lwc > 0) {
        float half_lwc = total_lwc * 0.5f;
        for (int i = 0; i < DSD_NUM_BINS; ++i) {
            if (lwc_cumulative[i] >= half_lwc) {
                float prev = (i > 0) ? lwc_cumulative[i - 1] : 0;
                float frac = (half_lwc - prev) / (lwc_cumulative[i] - prev + 1e-12f);
                dsd->median_diameter = DSD_BIN_EDGES[i] + frac
                                     * (DSD_BIN_EDGES[i + 1] - DSD_BIN_EDGES[i]);
                break;
            }
        }
    } else {
        dsd->median_diameter = 0;
    }

    /* Z-R relationship fit: Z = a · R^b
     * For a single interval we can't fit a and b (need multiple points).
     * Instead we report the instantaneous ratio and use the Marshall-Palmer
     * default b = 1.6, solving for a:
     *   a = Z / R^b */
    if (total_r > 0.01f) {
        dsd->zr_b = 1.6f;   /* Marshall-Palmer default */
        dsd->zr_a = total_z / powf(total_r, dsd->zr_b);
    } else {
        dsd->zr_a = 200.0f;  /* default */
        dsd->zr_b = 1.6f;
    }

    /* Average charge */
    if (total_count > 0)
        dsd->avg_charge_pc /= (int16_t)total_count;
}