/*
 * lockin.h — Digital lock-in detection for EIS impedance measurement
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_LOCKIN_H
#define LITHOCORE_LOCKIN_H

#include <stdint.h>
#include "ads1256.h"

/* Q1.31 fixed-point type — 31 fractional bits, range [-1.0, +1.0) */
typedef int32_t q31_t;

/* Complex impedance result at a single frequency */
typedef struct {
    uint32_t freq_hz;        /* measurement frequency */
    q31_t    re_z;           /* Re(Z) in Ohms (Q1.31 scaled) */
    q31_t    im_z;           /* Im(Z) in Ohms (Q1.31 scaled) */
    q31_t    mag_z;          /* |Z| in Ohms (Q1.31 scaled) */
    q31_t    phase_mdeg;     /* phase in millidegrees (Q1.31) */
    q31_t    re_v;           /* Re(V) component */
    q31_t    im_v;           /* Im(V) component */
    q31_t    re_i;           /* Re(I) component */
    q31_t    im_i;           /* Im(I) component */
    uint32_t samples;        /* number of samples averaged */
    uint8_t  valid;          /* 1 if measurement is valid */
} lockin_result_t;

/* API */
int  lockin_init(void);

/* Compute complex impedance from V and I capture buffers.
 *
 * The lock-in detection computes the in-phase (I) and quadrature (Q)
 * components of both V and I by multiplying each sample by the reference
 * sine and cosine (derived from the DDS frequency and phase), then
 * low-pass filtering (boxcar average). The complex impedance is:
 *
 *   Z = (V_I + j*V_Q) / (I_I + j*I_Q)
 *
 * This uses the CORDIC hardware accelerator for the complex divide.
 *
 * Author: jayis1 */
int  lockin_compute(const ads1256_capture_t *v_cap,
                    const ads1256_capture_t *i_cap,
                    uint32_t ref_freq_hz,
                    uint32_t ref_phase_accum,
                    lockin_result_t *result);

/* Low-frequency path: uses ADS1256 data */
int  lockin_measure_lf(uint32_t freq_hz, lockin_result_t *result);

/* High-frequency path: uses MCU ADC at 500 kSPS with oversampling */
int  lockin_measure_hf(uint32_t freq_hz, lockin_result_t *result);

/* CORDIC helpers */
q31_t cordic_cos(q31_t angle);    /* angle in Q1.31 radians [-π, +π) */
q31_t cordic_sin(q31_t angle);
q31_t cordic_atan2(q31_t y, q31_t x);
q31_t cordic_magnitude(q31_t x, q31_t y);

/* Q-format arithmetic */
q31_t q31_mul(q31_t a, q31_t b);
q31_t q31_div(q31_t a, q31_t b);

#endif /* LITHOCORE_LOCKIN_H */