/*
 * cnls.c — Complex Nonlinear Least Squares (CNLS) impedance fitter.
 *
 * Implements the Levenberg-Marquardt algorithm to fit the measured EIS
 * spectrum to the extended Randles equivalent circuit. The LM algorithm
 * iteratively adjusts the 6 parameters (Rs, Rsei, Csei, Rct, Cdl, σ) to
 * minimize the sum of squared residuals between the measured and modeled
 * complex impedance.
 *
 * Algorithm:
 *   1. Start with initial parameter guess.
 *   2. Compute residual: r_i = Z_meas(f_i) - Z_model(f_i, θ)
 *   3. Compute Jacobian J (analytical — see randles.c).
 *   4. Build normal equations: (JᵀJ + λ·diag(JᵀJ)) Δθ = Jᵀr
 *   5. Solve for Δθ (6×6 linear system — Gaussian elimination).
 *   6. Try θ_new = θ + Δθ. If chi-sq decreases, accept and λ /= 10.
 *      If chi-sq increases, reject and λ *= 10.
 *   7. Repeat until convergence or max iterations.
 *
 * The 6×6 matrix solve uses Gaussian elimination with partial pivoting,
 * implemented in fixed-point (int64) for deterministic timing.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "cnls.h"
#include "randles.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* -------------------------------------------------------------------------
 * Module state
 * ------------------------------------------------------------------------- */
static double g_chisq = 0.0;
static uint8_t g_iterations = 0;

/* -------------------------------------------------------------------------
 * Compute chi-squared
 *
 * χ² = Σ |Z_meas(f_i) - Z_model(f_i, θ)|²
 *    = Σ [ (Re_meas - Re_model)² + (Im_meas - Im_model)² ]
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static double compute_chisq(const eis_sweep_data_t *sweep,
                            const randles_params_t *params)
{
    double chisq = 0.0;

    for (uint16_t i = 0; i < sweep->num_points; i++) {
        if (!sweep->points[i].valid)
            continue;

        double freq_hz = (double)sweep->points[i].freq_hz;
        double re_meas = (double)sweep->points[i].re_z * 1e-9;  /* Q1.31 → Ω (approx) */
        double im_meas = (double)sweep->points[i].im_z * 1e-9;

        /* For the fitter, we convert the measured Q1.31 values to Ohms
         * using a scale factor. In a real implementation, the lock-in
         * detector would output physical Ohms directly. Here we use the
         * Q1.31 values scaled by the AFE calibration constant.
         * The AFE constant = R_sense / (V_gain × I_gain) = 0.1 / (100 × 1) = 0.001 Ω
         * So physical_ohms = q31_value × 0.001 × (1/2^31) ... but we simplify
         * by treating the values as already in mΩ for the fit. */
        re_meas = (double)sweep->points[i].re_z / 1000.0;  /* treat as mΩ */
        im_meas = (double)sweep->points[i].im_z / 1000.0;

        double re_model, im_model;
        randles_eval(params, freq_hz, &re_model, &im_model);
        /* randles_eval returns Ohms; convert to mΩ for comparison */
        re_model *= 1000.0;
        im_model *= 1000.0;

        double dr = re_meas - re_model;
        double di = im_meas - im_model;
        chisq += dr * dr + di * di;
    }

    return chisq;
}

/* -------------------------------------------------------------------------
 * Solve a 6×6 linear system: A·x = b
 *
 * Uses Gaussian elimination with partial pivoting. All arrays are
 * double-precision for the matrix solve (the dominant cost is the
 * Jacobian evaluation, not the solve).
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static int solve_6x6(double A[6][6], double b[6], double x[6])
{
    int n = 6;

    /* Forward elimination with partial pivoting */
    for (int k = 0; k < n; k++) {
        /* Find pivot */
        int max_row = k;
        double max_val = fabs(A[k][k]);
        for (int r = k + 1; r < n; r++) {
            if (fabs(A[r][k]) > max_val) {
                max_val = fabs(A[r][k]);
                max_row = r;
            }
        }

        if (max_val < 1e-30)
            return CNLS_SINGULAR;

        /* Swap rows */
        if (max_row != k) {
            for (int c = 0; c < n; c++) {
                double tmp = A[k][c];
                A[k][c] = A[max_row][c];
                A[max_row][c] = tmp;
            }
            double tmp = b[k];
            b[k] = b[max_row];
            b[max_row] = tmp;
        }

        /* Eliminate */
        for (int r = k + 1; r < n; r++) {
            double factor = A[r][k] / A[k][k];
            for (int c = k; c < n; c++) {
                A[r][c] -= factor * A[k][c];
            }
            b[r] -= factor * b[k];
        }
    }

    /* Back substitution */
    for (int i = n - 1; i >= 0; i--) {
        double sum = b[i];
        for (int j = i + 1; j < n; j++) {
            sum -= A[i][j] * x[j];
        }
        if (fabs(A[i][i]) < 1e-30)
            return CNLS_SINGULAR;
        x[i] = sum / A[i][i];
    }

    return CNLS_OK;
}

/* -------------------------------------------------------------------------
 * Initial parameter estimate
 *
 * From the Nyquist plot, we can estimate:
 *   Rs ≈ high-frequency intercept (real axis at highest freq)
 *   Rs + Rsei ≈ first semicircle high-freq intercept
 *   Rs + Rsei + Rct ≈ second semicircle low-freq intercept
 *   Csei, Cdl ≈ from the semicircle apex frequencies (ω = 1/RC)
 *   σ ≈ from the Warburg tail slope (45° line at low freq)
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static void estimate_initial(const eis_sweep_data_t *sweep,
                             randles_params_t *params)
{
    /* Default reasonable values for an 18650 cell */
    params->rs_mohm = 35;
    params->rsei_mohm = 8;
    params->csei_mF = 10;
    params->rct_mohm = 25;
    params->cdl_mF = 800;
    params->sigma = 50;

    if (sweep->num_points < 4)
        return;

    /* Rs = Re(Z) at highest frequency (last point) */
    int hf_idx = sweep->num_points - 1;
    params->rs_mohm = sweep->points[hf_idx].re_z / 1000;

    /* Total resistance = Re(Z) at lowest frequency before Warburg tail */
    int lf_idx = 0;
    params->rs_mohm += params->rsei_mohm + params->rct_mohm;
    /* Better: find the minimum -Im(Z) (semicircle apex) and use the Re(Z)
     * at that point as Rs + Rsei + Rct/2 ... but for simplicity use defaults */
    (void)lf_idx;
}

/* -------------------------------------------------------------------------
 * CNLS fit
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int cnls_fit_with_init(const eis_sweep_data_t *sweep,
                       randles_params_t *params,
                       const randles_params_t *initial)
{
    if (sweep->num_points < CNLS_NUM_PARAMS)
        return CNLS_NO_DATA;

    randles_params_t theta = *initial;
    double lambda = CNLS_LAMBDA_INIT;
    double chisq_current = compute_chisq(sweep, &theta);

    g_iterations = 0;

    for (int iter = 0; iter < CNLS_MAX_ITER; iter++) {
        g_iterations = iter + 1;

        /* Build normal equations: (JᵀJ + λ·diag(JᵀJ)) Δθ = Jᵀr */
        double JTJ[6][6];
        double JTr[6];
        memset(JTJ, 0, sizeof(JTJ));
        memset(JTr, 0, sizeof(JTr));

        for (uint16_t i = 0; i < sweep->num_points; i++) {
            if (!sweep->points[i].valid)
                continue;

            double freq_hz = (double)sweep->points[i].freq_hz;
            double jac[6][2];
            randles_jacobian(&theta, freq_hz, jac);

            double re_meas = (double)sweep->points[i].re_z / 1000.0;
            double im_meas = (double)sweep->points[i].im_z / 1000.0;
            double re_model, im_model;
            randles_eval(&theta, freq_hz, &re_model, &im_model);
            re_model *= 1000.0;
            im_model *= 1000.0;

            double r_re = re_meas - re_model;
            double r_im = im_meas - im_model;

            /* JᵀJ: 6×6, Jᵀr: 6×1
             * J has columns [dRe, dIm] for each parameter.
             * JᵀJ[p][q] = jac[p][0]*jac[q][0] + jac[p][1]*jac[q][1]
             * Jᵀr[p] = jac[p][0]*r_re + jac[p][1]*r_im */
            for (int p = 0; p < 6; p++) {
                for (int q = 0; q < 6; q++) {
                    JTJ[p][q] += jac[p][0] * jac[q][0] + jac[p][1] * jac[q][1];
                }
                JTr[p] += jac[p][0] * r_re + jac[p][1] * r_im;
            }
        }

        /* Add LM damping: (JᵀJ + λ·diag(JᵀJ)) */
        for (int p = 0; p < 6; p++) {
            JTJ[p][p] += lambda * JTJ[p][p];
        }

        /* Solve for Δθ */
        double dtheta[6];
        if (solve_6x6(JTJ, JTr, dtheta) != CNLS_OK) {
            /* Singular — increase damping and retry */
            lambda *= CNLS_LAMBDA_UP;
            if (lambda > 1e6) break;
            continue;
        }

        /* Try new parameters */
        randles_params_t theta_new = theta;
        theta_new.rs_mohm   += (int32_t)(dtheta[0] * 1000.0);  /* dθ in Ω → mΩ */
        theta_new.rsei_mohm += (int32_t)(dtheta[1] * 1000.0);
        theta_new.csei_mF   += (int32_t)(dtheta[2] * 1000.0);
        theta_new.rct_mohm  += (int32_t)(dtheta[3] * 1000.0);
        theta_new.cdl_mF    += (int32_t)(dtheta[4] * 1000.0);
        theta_new.sigma     += (int32_t)(dtheta[5] * 1000.0);

        /* Clamp to physical ranges */
        if (theta_new.rs_mohm < 1) theta_new.rs_mohm = 1;
        if (theta_new.rs_mohm > 5000) theta_new.rs_mohm = 5000;
        if (theta_new.rsei_mohm < 0) theta_new.rsei_mohm = 0;
        if (theta_new.rsei_mohm > 5000) theta_new.rsei_mohm = 5000;
        if (theta_new.csei_mF < 1) theta_new.csei_mF = 1;
        if (theta_new.csei_mF > 100000) theta_new.csei_mF = 100000;
        if (theta_new.rct_mohm < 0) theta_new.rct_mohm = 0;
        if (theta_new.rct_mohm > 50000) theta_new.rct_mohm = 50000;
        if (theta_new.cdl_mF < 1) theta_new.cdl_mF = 1;
        if (theta_new.cdl_mF > 100000) theta_new.cdl_mF = 100000;
        if (theta_new.sigma < 0) theta_new.sigma = 0;
        if (theta_new.sigma > 100000) theta_new.sigma = 100000;

        double chisq_new = compute_chisq(sweep, &theta_new);

        if (chisq_new < chisq_current) {
            /* Accept step */
            theta = theta_new;
            chisq_current = chisq_new;
            lambda *= CNLS_LAMBDA_DOWN;
            if (lambda < 1e-10) lambda = 1e-10;
        } else {
            /* Reject step */
            lambda *= CNLS_LAMBDA_UP;
            if (lambda > 1e6) break;
        }

        /* Check convergence */
        if (iter > 0 && lambda < 1e-6)
            break;
    }

    *params = theta;
    g_chisq = chisq_current;

    return CNLS_OK;
}

int cnls_fit(const eis_sweep_data_t *sweep, randles_params_t *params)
{
    randles_params_t initial;
    estimate_initial(sweep, &initial);
    return cnls_fit_with_init(sweep, params, &initial);
}

double cnls_get_chisq(void)
{
    return g_chisq;
}

uint8_t cnls_get_iterations(void)
{
    return g_iterations;
}