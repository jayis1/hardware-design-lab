/*
 * cnls.h — Complex Nonlinear Least Squares (CNLS) impedance fitter
 *
 * Fits measured EIS data to the extended Randles equivalent circuit
 * using the Levenberg-Marquardt algorithm.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_CNLS_H
#define LITHOCORE_CNLS_H

#include <stdint.h>
#include "randles.h"
#include "eis_sweep.h"

/* Return codes */
#define CNLS_OK            0
#define CNLS_NO_DATA      -1
#define CNLS_NO_CONVERGE  -2
#define CNLS_SINGULAR     -3

/* Fitter configuration */
#define CNLS_MAX_ITER      20
#define CNLS_NUM_PARAMS    6     /* Rs, Rsei, Csei, Rct, Cdl, σ */
#define CNLS_LAMBDA_INIT   0.01
#define CNLS_LAMBDA_UP     10.0
#define CNLS_LAMBDA_DOWN   0.1
#define CNLS_TOL           1e-6

/* API */
int cnls_fit(const eis_sweep_data_t *sweep, randles_params_t *params);
int cnls_fit_with_init(const eis_sweep_data_t *sweep,
                       randles_params_t *params,
                       const randles_params_t *initial);

/* Get the final chi-squared (goodness of fit) */
double cnls_get_chisq(void);

/* Get the number of iterations used */
uint8_t cnls_get_iterations(void);

#endif /* LITHOCORE_CNLS_H */