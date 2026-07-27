/*
 * randles.c — Extended Randles equivalent-circuit model evaluation.
 *
 * Provides both floating-point (for initial estimation) and fixed-point
 * (for the CNLS fitter) evaluation of the cell impedance model, plus
 * the analytical Jacobian needed for Levenberg-Marquardt fitting.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <math.h>
#include "randles.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------------
 * Floating-point model evaluation
 *
 * Z(f) = Rs + Rsei/(1+jωRseiCsei) + Rct/(1+jωRctCdl) + σ(1-j)/√(2πf)
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void randles_eval(const randles_params_t *p, double freq_hz,
                  double *re_z, double *im_z)
{
    double omega = 2.0 * M_PI * freq_hz;

    /* Rs (purely real) */
    double rs = (double)p->rs_mohm * 1e-3;  /* mΩ → Ω */

    /* SEI RC element: Rsei/(1 + jωRseiCsei) */
    double rsei = (double)p->rsei_mohm * 1e-3;
    double csei = (double)p->csei_mF * 1e-3;     /* mF → F */
    double wrc_sei = omega * rsei * csei;
    double re_sei = rsei / (1.0 + wrc_sei * wrc_sei);
    double im_sei = -omega * rsei * rsei * csei / (1.0 + wrc_sei * wrc_sei);

    /* Charge-transfer RC element: Rct/(1 + jωRctCdl) */
    double rct = (double)p->rct_mohm * 1e-3;
    double cdl = (double)p->cdl_mF * 1e-3;
    double wrc_ct = omega * rct * cdl;
    double re_ct = rct / (1.0 + wrc_ct * wrc_ct);
    double im_ct = -omega * rct * rct * cdl / (1.0 + wrc_ct * wrc_ct);

    /* Warburg element: σ(1-j)/√(2πf) */
    double sigma = (double)p->sigma * 1e-3;  /* mΩ/√Hz → Ω/√Hz */
    double sqrt_2pi_f = sqrt(2.0 * M_PI * freq_hz);
    double re_w = sigma / sqrt_2pi_f;
    double im_w = -sigma / sqrt_2pi_f;

    /* Sum all elements */
    *re_z = rs + re_sei + re_ct + re_w;
    *im_z = im_sei + im_ct + im_w;
}

/* -------------------------------------------------------------------------
 * Fixed-point model evaluation (Q1.31 style, but using int64 for
 * intermediate values to avoid overflow).
 *
 * This is used by the CNLS fitter which operates entirely in fixed-point
 * to exploit the CORDIC/FMAC hardware and ensure deterministic timing.
 *
 * Frequency is in milli-Hz, impedance output in micro-ohms (to preserve
 * resolution for small resistances).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void randles_eval_q(const randles_params_t *p, uint32_t freq_mhz,
                    int64_t *re_z, int64_t *im_z)
{
    /* Convert to physical units for computation.
     * freq_mhz is in milli-Hz; freq_hz = freq_mhz / 1000.
     * Parameters are in mΩ and mF.
     *
     * We compute in micro-ohms (µΩ) = mΩ × 1000 to maintain integer precision.
     *
     * omega = 2π × freq_hz = 2π × freq_mhz / 1000
     *       = 6283 × freq_mhz / 1000000  (using 2π ≈ 6.283)
     *
     * For the RC element (R in µΩ, C in mF = 1e-3 F):
     *   ωRC = omega × R × C = omega × (R_mohm × 1000) × (C_mF × 1e-3)
     *        = omega × R_mohm × C_mF
     *
     * To keep this integer-friendly, we scale omega by 1e6:
     *   omega_1e6 = 6283 × freq_mhz / 1000  (in units of rad/s × 1e6)
     * Then ωRC = (omega_1e6 × R_mohm × C_mF) / 1e6
     */
    int64_t freq_hz_x1000 = freq_mhz;  /* freq in mHz */
    int64_t omega_1e6 = 6283 * freq_hz_x1000 / 1000;  /* ω × 1e6 */

    /* Rs in µΩ */
    int64_t rs_uohm = (int64_t)p->rs_mohm * 1000;

    /* SEI RC: R in µΩ, C in mF
     * ωRC = (omega_1e6 × R_mohm × C_mF) / 1e6
     * Re = R / (1 + (ωRC)²)
     * Im = -ωR²C / (1 + (ωRC)²) = -Re × ωRC × (R/R) ... let's compute directly */
    int64_t rsei_mohm = p->rsei_mohm;
    int64_t csei_mF   = p->csei_mF;
    int64_t wrc_sei   = (omega_1e6 * rsei_mohm * csei_mF) / 1000000;
    int64_t wrc_sei_sq = wrc_sei * wrc_sei / 1000000;  /* (ωRC)² scaled by 1e6 */
    int64_t denom_sei = 1000000 + wrc_sei_sq;          /* 1 + (ωRC)², scaled by 1e6 */
    int64_t rsei_uohm = rsei_mohm * 1000;
    int64_t re_sei_uohm = (rsei_uohm * 1000000) / denom_sei;
    int64_t im_sei_uohm = -(re_sei_uohm * wrc_sei) / 1000000;

    /* Charge-transfer RC */
    int64_t rct_mohm = p->rct_mohm;
    int64_t cdl_mF   = p->cdl_mF;
    int64_t wrc_ct   = (omega_1e6 * rct_mohm * cdl_mF) / 1000000;
    int64_t wrc_ct_sq = wrc_ct * wrc_ct / 1000000;
    int64_t denom_ct = 1000000 + wrc_ct_sq;
    int64_t rct_uohm = rct_mohm * 1000;
    int64_t re_ct_uohm = (rct_uohm * 1000000) / denom_ct;
    int64_t im_ct_uohm = -(re_ct_uohm * wrc_ct) / 1000000;

    /* Warburg: σ(1-j)/√(2πf)
     * σ in µΩ/√Hz
     * √(2πf) = √(2π × freq_mhz/1000) = √(6.283 × freq_mhz / 1000)
     * We compute an integer approximation of 1/√(2πf) using a lookup table
     * or Newton's method. For simplicity, use a coarse fixed-point sqrt. */
    int64_t sigma_uohm = (int64_t)p->sigma * 1000;
    /* 2πf in mHz: 6283 × freq_mhz / 1000 (in mHz, need Hz: /1000) */
    int64_t two_pi_f_x1000 = 6283 * freq_hz_x1000 / 1000;  /* 2πf × 1000 */
    /* √(2πf) — integer sqrt of two_pi_f_x1000, then divide by 32 (≈√1000) */
    int64_t v = two_pi_f_x1000;
    if (v <= 0) v = 1;
    /* Newton's method integer sqrt */
    int64_t x = v;
    int64_t y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + v / x) / 2; }
    int64_t sqrt_val = x;  /* ≈ √(2πf × 1000) ≈ √(2πf) × 31.6 */
    int64_t inv_sqrt = 31600 / (sqrt_val > 0 ? sqrt_val : 1);  /* 1/√(2πf) × 1e6 */

    int64_t re_w_uohm = (sigma_uohm * inv_sqrt) / 1000000;
    int64_t im_w_uohm = -re_w_uohm;

    *re_z = rs_uohm + re_sei_uohm + re_ct_uohm + re_w_uohm;
    *im_z = im_sei_uohm + im_ct_uohm + im_w_uohm;
}

/* -------------------------------------------------------------------------
 * Analytical Jacobian
 *
 * Partial derivatives of Re(Z) and Im(Z) with respect to each parameter
 * [Rs, Rsei, Csei, Rct, Cdl, σ].
 *
 * For a parallel RC element: Z = R/(1+jωRC)
 *   Let x = ωRC
 *   Re(Z) = R/(1+x²),  Im(Z) = -ωR²C/(1+x²) = -Rx/(1+x²)
 *
 *   dRe/dR = (1+x²-R·2x·ωC)/(1+x²)² = (1-x²)/(1+x²)²
 *   dRe/dC = -R·2x·ωR/(1+x²)² = -2ωR²x/(1+x²)²
 *   dIm/dR = -ωC(1+x²-2x²)/(1+x²)² = -ωC(1-x²)/(1+x²)²
 *   dIm/dC = -ωR²(1+x²-2x·ωRC)/(1+x²)² = -ωR²(1-x²)/(1+x²)²
 *
 * For Warburg: Z_W = σ(1-j)/√(2πf)
 *   dRe/dσ = 1/√(2πf),  dIm/dσ = -1/√(2πf)
 *
 * For Rs: dRe/dRs = 1, dIm/dRs = 0
 *
 * Jacobian[i] = [dRe/dparam_i, dIm/dparam_i]
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void randles_jacobian(const randles_params_t *p, double freq_hz,
                      double jacobian[6][2])
{
    double omega = 2.0 * M_PI * freq_hz;

    /* Rs */
    jacobian[0][0] = 1.0;   /* dRe/dRs */
    jacobian[0][1] = 0.0;   /* dIm/dRs */

    /* SEI: Rsei, Csei */
    double rsei = p->rsei_mohm * 1e-3;
    double csei = p->csei_mF * 1e-3;
    double x_sei = omega * rsei * csei;
    double d_sei = 1.0 + x_sei * x_sei;
    jacobian[1][0] = (1.0 - x_sei * x_sei) / (d_sei * d_sei);  /* dRe/dRsei */
    jacobian[1][1] = -omega * csei * (1.0 - x_sei * x_sei) / (d_sei * d_sei);  /* dIm/dRsei */
    jacobian[2][0] = -2.0 * omega * rsei * rsei * x_sei / (d_sei * d_sei);     /* dRe/dCsei */
    jacobian[2][1] = -omega * rsei * rsei * (1.0 - x_sei * x_sei) / (d_sei * d_sei); /* dIm/dCsei */

    /* CT: Rct, Cdl */
    double rct = p->rct_mohm * 1e-3;
    double cdl = p->cdl_mF * 1e-3;
    double x_ct = omega * rct * cdl;
    double d_ct = 1.0 + x_ct * x_ct;
    jacobian[3][0] = (1.0 - x_ct * x_ct) / (d_ct * d_ct);     /* dRe/dRct */
    jacobian[3][1] = -omega * cdl * (1.0 - x_ct * x_ct) / (d_ct * d_ct);  /* dIm/dRct */
    jacobian[4][0] = -2.0 * omega * rct * rct * x_ct / (d_ct * d_ct);     /* dRe/dCdl */
    jacobian[4][1] = -omega * rct * rct * (1.0 - x_ct * x_ct) / (d_ct * d_ct);  /* dIm/dCdl */

    /* Warburg: σ */
    double inv_sqrt = 1.0 / sqrt(2.0 * M_PI * freq_hz);
    jacobian[5][0] = inv_sqrt;     /* dRe/dσ */
    jacobian[5][1] = -inv_sqrt;    /* dIm/dσ */
}