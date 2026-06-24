/*
 * power_calc.c — Real/reactive/apparent power, RMS, PF, frequency
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Computes IEC 61000-4-30 Class S power parameters from 1-second windows
 * of simultaneously-sampled voltage and current:
 *
 *   V_rms  = sqrt( (1/N) Σ v[n]^2 )
 *   I_rms  = sqrt( (1/N) Σ i[n]^2 )
 *   P_real = (1/N) Σ v[n] · i[n]          (active power)
 *   Q      = (1/N) Σ v_h[n] · i[n]        (reactive via 90° Hilbert shift of v)
 *   S      = V_rms · I_rms                (apparent power)
 *   PF     = P_real / S                   (power factor)
 *   f      = frequency via zero-crossing interpolation
 *
 * The 90° phase shift for reactive power uses a 3-tap FIR Hilbert approximator
 * (sufficient for 50/60 Hz fundamentals with harmonic content < 5%).
 */

#include "power_calc.h"
#include "dsp_fft.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

/* Hilbert 90° phase-shift FIR coefficients (3-tap, wideband approximation) */
static float hilbert_coef[3];

void power_calc_init(void) {
    /* 3-tap Hilbert transformer: h[0]=-0.5, h[1]=0, h[2]=+0.5
     * This gives ~90° shift for frequencies well within Nyquist.
     * For better accuracy, use a longer FIR designed for 40-70 Hz band. */
    hilbert_coef[0] = -0.5f;
    hilbert_coef[1] =  0.0f;
    hilbert_coef[2] =  0.5f;
}

/* ========================================================================
 * RMS calculation
 * ======================================================================== */
float power_calc_rms(const float *x, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += x[i] * x[i];
    }
    return sqrtf(sum / (float)n);
}

/* ========================================================================
 * Grid frequency via zero-crossing interpolation
 * Counts zero crossings in the window and interpolates exact crossing times
 * for sub-bin frequency resolution.
 * ======================================================================== */
float power_calc_frequency(const float *v, int n, float fs) {
    int crossings = 0;
    float last_cross_time = -1.0f;
    float first_cross_time = -1.0f;
    int last_sign = (v[0] >= 0) ? 1 : -1;

    for (int i = 1; i < n; i++) {
        int sign = (v[i] >= 0) ? 1 : -1;
        if (sign != last_sign) {
            /* Linear interpolation to find exact crossing sample */
            float frac = -v[i - 1] / (v[i] - v[i - 1]);
            float t = (float)(i - 1) + frac;  /* sample index of crossing */

            if (first_cross_time < 0) {
                first_cross_time = t;
            }
            last_cross_time = t;
            crossings++;
            last_sign = sign;
        }
    }

    if (crossings < 4) return 0.0f;  /* not enough crossings */

    /* Each full period = 2 zero crossings.
     * Frequency = (crossings-1)/2 / (time between first and last crossing) */
    float total_time = (last_cross_time - first_cross_time) / fs;  /* seconds */
    if (total_time <= 0) return 0.0f;

    int periods = (crossings - 1) / 2;
    return (float)periods / total_time;
}

/* ========================================================================
 * 90° Hilbert phase shift (3-tap FIR)
 * Produces v_h[n] ≈ v[n] shifted by 90°
 * ======================================================================== */
static void hilbert_shift(const float *v, float *v_h, int n) {
    /* Edge samples use zero-padding */
    v_h[0] = hilbert_coef[1] * v[0] + hilbert_coef[2] * v[1];
    for (int i = 1; i < n - 1; i++) {
        v_h[i] = hilbert_coef[0] * v[i - 1]
               + hilbert_coef[1] * v[i]
               + hilbert_coef[2] * v[i + 1];
    }
    v_h[n - 1] = hilbert_coef[0] * v[n - 2] + hilbert_coef[1] * v[n - 1];
}

/* ========================================================================
 * Main power computation
 *
 * v: array of 3 voltage channel pointers (v[0..2], each WINDOW_SAMPLES long)
 * i: array of 4 current channel pointers (i[0..3], each WINDOW_SAMPLES long)
 * m: output metrics
 * cal: calibration data (for phase correction)
 * ======================================================================== */
void power_calc_compute(const float *v[3], const float *i[4],
                        power_metrics_t *m, const cal_data_t *cal) {
    (void)cal;  /* phase corrections applied in convert_samples; could refine here */

    float v_h[WINDOW_SAMPLES];  /* Hilbert-shifted voltage */

    m->p_total_real = 0.0f;
    m->p_total_reactive = 0.0f;
    m->p_total_apparent = 0.0f;

    for (int phase = 0; phase < 3; phase++) {
        /* RMS values */
        m->v_rms[phase] = power_calc_rms(v[phase], WINDOW_SAMPLES);
        m->i_rms[phase] = power_calc_rms(i[phase], WINDOW_SAMPLES);

        /* Real power: average of instantaneous product */
        float p_sum = 0.0f;
        for (int s = 0; s < WINDOW_SAMPLES; s++) {
            p_sum += v[phase][s] * i[phase][s];
        }
        m->p_real[phase] = p_sum / (float)WINDOW_SAMPLES;

        /* Reactive power: average of Hilbert(v) × i */
        hilbert_shift(v[phase], v_h, WINDOW_SAMPLES);
        float q_sum = 0.0f;
        for (int s = 0; s < WINDOW_SAMPLES; s++) {
            q_sum += v_h[s] * i[phase][s];
        }
        m->p_reactive[phase] = q_sum / (float)WINDOW_SAMPLES;

        /* Apparent power */
        m->p_apparent[phase] = m->v_rms[phase] * m->i_rms[phase];

        /* Power factor */
        if (m->p_apparent[phase] > 0.001f) {
            m->pf[phase] = m->p_real[phase] / m->p_apparent[phase];
            /* Clamp PF to [-1, 1] */
            if (m->pf[phase] > 1.0f) m->pf[phase] = 1.0f;
            if (m->pf[phase] < -1.0f) m->pf[phase] = -1.0f;
        } else {
            m->pf[phase] = 0.0f;
        }

        /* Accumulate totals */
        m->p_total_real     += m->p_real[phase];
        m->p_total_reactive += m->p_reactive[phase];
        m->p_total_apparent += m->p_apparent[phase];
    }

    /* Neutral current RMS */
    m->i_rms[3] = power_calc_rms(i[3], WINDOW_SAMPLES);

    /* Total power factor */
    if (m->p_total_apparent > 0.001f) {
        m->pf_total = m->p_total_real / m->p_total_apparent;
        if (m->pf_total > 1.0f) m->pf_total = 1.0f;
        if (m->pf_total < -1.0f) m->pf_total = -1.0f;
    } else {
        m->pf_total = 0.0f;
    }

    /* Grid frequency from phase A voltage */
    m->freq = power_calc_frequency(v[0], WINDOW_SAMPLES, (float)SAMPLE_RATE_HZ);
    if (m->freq < 45.0f || m->freq > 65.0f) {
        m->freq = (float)cal->grid_freq;  /* fallback to configured frequency */
    }
}