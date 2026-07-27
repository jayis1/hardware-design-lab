/*
 * randles.h — Extended Randles equivalent-circuit model
 *
 * The extended Randles model for a lithium-ion cell:
 *
 *   Z(f) = Rs + Rsei/(1 + jω·Rsei·Csei) + Rct/(1 + jω·Rct·Cdl) + Zw(f)
 *
 * where Zw(f) = σ·(1-j) / √(2πf)  is the Warburg diffusion element.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_RANDLES_H
#define LITHOCORE_RANDLES_H

#include <stdint.h>
#include "lockin.h"

/* Randles equivalent circuit parameters */
typedef struct {
    /* All values in milli-ohms / milli-Farads for fixed-point convenience */
    int32_t rs_mohm;       /* solution/electrolyte resistance (mΩ) */
    int32_t rsei_mohm;     /* SEI layer resistance (mΩ) */
    int32_t csei_mF;       /* SEI layer capacitance (mF) */
    int32_t rct_mohm;      /* charge-transfer resistance (mΩ) */
    int32_t cdl_mF;        /* double-layer capacitance (mF) */
    int32_t sigma;         /* Warburg coefficient (mΩ/√Hz) */
} randles_params_t;

/* Evaluate the Randles model at a given frequency.
 * Returns Re(Z) and Im(Z) in milli-ohms.
 *
 * Z(f) = Rs + Rsei/(1+jωRseiCsei) + Rct/(1+jωRctCdl) + σ(1-j)/√(2πf)
 *
 * Each RC parallel element: Z_RC = R / (1 + jωRC)
 *   Re(Z_RC) = R / (1 + (ωRC)²)
 *   Im(Z_RC) = -ωR²C / (1 + (ωRC)²)
 *
 * Warburg: Z_W = σ(1-j) / √(2πf)
 *   Re(Z_W) = σ / √(2πf)
 *   Im(Z_W) = -σ / √(2πf)
 *
 * Author: jayis1 */
void randles_eval(const randles_params_t *p, double freq_hz,
                  double *re_z, double *im_z);

/* Evaluate using fixed-point (Q1.31) — used by the CNLS fitter */
void randles_eval_q(const randles_params_t *p, uint32_t freq_mhz,
                    int64_t *re_z, int64_t *im_z);

/* Compute the Jacobian (partial derivatives of Re(Z) and Im(Z) w.r.t.
 * each parameter) — used by the Levenberg-Marquardt fitter. */
void randles_jacobian(const randles_params_t *p, double freq_hz,
                      double jacobian[6][2]);

#endif /* LITHOCORE_RANDLES_H */