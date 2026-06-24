/*
 * flicker.c — IEC 61000-4-15 flicker Pst digital filter chain
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Implements the IEC 61000-4-15 flickermeter block diagram:
 *
 *   1. Voltage adaptor      — normalizes input to 1.0 p.u.
 *   2. Squaring demodulator — v^2(t) extracts the envelope (modulation)
 *   3. Weighting filter     — 1st-order HP + 6th-order LP + weighting (eye-brain)
 *   4. Squaring + smoothing — instantaneous flicker perception
 *   5. Statistical eval     — CPF (cumulative probability function) → Pst
 *
 * Pst is computed over a 10-minute window (per standard) but we provide
 * a 1-second "short-term flicker indicator" for real-time display and
 * accumulate for the full 10-minute Pst.
 *
 * The weighting filter is implemented as cascaded biquads (Direct Form II).
 */

#include "flicker.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define PST_WINDOW_S    600    /* 10 minutes = 600 seconds */
#define PST_SHORT_S     1      /* short-term (1 s) indicator */
#define IEC_LAMPS_230   0      /* 230 V lamp model */

/* ========================================================================
 * Biquad filter (Direct Form II Transposed)
 * ======================================================================== */
typedef struct {
    float b0, b1, b2, a1, a2;
    float z1, z2;
} biquad_t;

static inline float biquad_process(biquad_t *f, float x) {
    float y = f->b0 * x + f->z1;
    f->z1 = f->b1 * x - f->a1 * y + f->z2;
    f->z2 = f->b2 * x - f->a2 * y;
    return y;
}

/* ========================================================================
 * IEC 61000-4-15 weighting filter coefficients
 * Designed for 230 V / 50 Hz lamp (coefficients at 2048 Sa/s)
 * ======================================================================== */

/* High-pass: 0.05 Hz cutoff (removes DC from squaring demodulator) */
static biquad_t hp_filter = {
    .b0 =  0.9993, .b1 = -1.9986, .b2 =  0.9993,
    .a1 = -1.9986, .a2 =  0.9986,
    .z1 = 0, .z2 = 0
};

/* Low-pass: 6th-order Butterworth, 35 Hz cutoff (removes carrier harmonics) */
static biquad_t lp1 = {
    .b0 =  0.0024, .b1 =  0.0048, .b2 =  0.0024,
    .a1 = -1.7996, .a2 =  0.8091,
    .z1 = 0, .z2 = 0
};
static biquad_t lp2 = {
    .b0 =  1.0,    .b1 =  2.0,    .b2 =  1.0,
    .a1 = -1.5404, .a2 =  0.6361,
    .z1 = 0, .z2 = 0
};
static biquad_t lp3 = {
    .b0 =  1.0,    .b1 =  2.0,    .b2 =  1.0,
    .a1 = -1.2409, .a2 =  0.4011,
    .z1 = 0, .z2 = 0
};

/* Weighting filter (eye-brain lamp response) — 2 biquads */
static biquad_t w1 = {
    .b0 =  0.5457, .b1 = -0.5849, .b2 =  0.0,
    .a1 = -0.5849, .a2 =  0.0,
    .z1 = 0, .z2 = 0
};
static biquad_t w2 = {
    .b0 =  3.2814, .b1 =  3.2814, .b2 =  0.0,
    .a1 = -0.7493, .a2 =  0.0,
    .z1 = 0, .z2 = 0
};

/* Per-phase filter states (3 phases) */
typedef struct {
    biquad_t hp, l1, l2, l3, wt1, wt2;
    float voltage_ref;             /* running RMS reference for 1 p.u. normalization */
    float ifeel_buf[PST_WINDOW_S]; /* instantaneous flicker perception samples */
    int    ifeel_idx;
    float  pst_short;              /* 1-second short-term indicator */
    float  pst_10min;              /* 10-minute Pst */
} flicker_phase_t;

static flicker_phase_t fph[3];

/* ========================================================================
 * Initialize flicker filter chain
 * ======================================================================== */
void flicker_init(void) {
    for (int p = 0; p < 3; p++) {
        memcpy(&fph[p].hp, &hp_filter, sizeof(biquad_t));
        memcpy(&fph[p].l1, &lp1, sizeof(biquad_t));
        memcpy(&fph[p].l2, &lp2, sizeof(biquad_t));
        memcpy(&fph[p].l3, &lp3, sizeof(biquad_t));
        memcpy(&fph[p].wt1, &w1, sizeof(biquad_t));
        memcpy(&fph[p].wt2, &w2, sizeof(biquad_t));
        fph[p].voltage_ref = 230.0f;
        fph[p].ifeel_idx = 0;
        fph[p].pst_short = 0.0f;
        fph[p].pst_10min = 0.0f;
        memset(fph[p].ifeel_buf, 0, sizeof(fph[p].ifeel_buf));
    }
}

/* ========================================================================
 * Process one 1-second window through the flicker filter chain
 *
 * Block 1: Voltage adaptor — normalize to 1 p.u. (divide by RMS)
 * Block 2: Squaring demodulator — v_norm^2
 * Block 3: Weighting filter — HP + LP(6th) + weighting
 * Block 4: Squaring + smoothing — instantaneous flicker perception
 * Block 5: Statistical evaluation — CPF → Pst (computed at 10-min boundary)
 * ======================================================================== */
void flicker_update(const float *v1, const float *v2, const float *v3, float grid_freq) {
    (void)grid_freq;
    const float *vp[3] = { v1, v2, v3 };

    for (int p = 0; p < 3; p++) {
        flicker_phase_t *fp = &fph[p];
        const float *v = vp[p];

        /* Compute running RMS for voltage adaptor (block 1) */
        float sq_sum = 0.0f;
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            sq_sum += v[i] * v[i];
        }
        float vrms = sqrtf(sq_sum / (float)WINDOW_SAMPLES);
        /* Exponential smoothing of reference voltage (time constant ~10 s) */
        fp->voltage_ref = 0.998f * fp->voltage_ref + 0.002f * vrms;
        float v_ref = fp->voltage_ref > 1.0f ? fp->voltage_ref : 230.0f;

        /* Process through filter chain */
        float ifeel_sum = 0.0f;
        for (int i = 0; i < WINDOW_SAMPLES; i++) {
            /* Block 1: normalize to 1 p.u. */
            float vn = v[i] / v_ref;

            /* Block 2: squaring demodulator */
            float sq = vn * vn;

            /* Block 3: weighting filter chain */
            float y = biquad_process(&fp->hp, sq);
            y = biquad_process(&fp->l1, y);
            y = biquad_process(&fp->l2, y);
            y = biquad_process(&fp->l3, y);
            y = biquad_process(&fp->wt1, y);
            y = biquad_process(&fp->wt2, y);

            /* Block 4: squaring + smoothing (instantaneous flicker perception) */
            float ifeel = y * y;
            ifeel_sum += ifeel;
        }

        /* Average instantaneous flicker over the 1-second window */
        float ifeel_avg = ifeel_sum / (float)WINDOW_SAMPLES;

        /* Store in circular buffer for Pst statistical evaluation */
        fp->ifeel_buf[fp->ifeel_idx] = ifeel_avg;
        fp->ifeel_idx = (fp->ifeel_idx + 1) % PST_WINDOW_S;

        /* Short-term (1-second) flicker indicator:
         * Pst(1s) = sqrt( average of ifeel^2 over 1s ) */
        fp->pst_short = sqrtf(ifeel_avg);

        /* If 10-minute window is full, compute full Pst via CPF method */
        if (fp->ifeel_idx == 0) {
            /* Sort the 600 samples for CPF (simplified: use percentile method) */
            float sorted[PST_WINDOW_S];
            memcpy(sorted, fp->ifeel_buf, sizeof(fp->ifeel_buf));
            /* Simple insertion sort (600 elements, runs once per 10 min) */
            for (int i = 1; i < PST_WINDOW_S; i++) {
                float key = sorted[i];
                int j = i - 1;
                while (j >= 0 && sorted[j] > key) {
                    sorted[j + 1] = sorted[j];
                    j--;
                }
                sorted[j + 1] = key;
            }

            /* IEC 61000-4-15 Pst from CPF percentiles:
             * Pst = sqrt(0.0314*P(0.1s) + 0.0525*P(1s) + 0.0657*P(3s) +
             *            0.28*P(10s) + 0.08*P(50s))
             * where P(x) = sqrt(percentile_x of ifeel samples)
             */
            int idx_01 = (int)(0.001f * PST_WINDOW_S);
            int idx_1  = (int)(0.01f * PST_WINDOW_S);
            int idx_3  = (int)(0.03f * PST_WINDOW_S);
            int idx_10 = (int)(0.10f * PST_WINDOW_S);
            int idx_50 = (int)(0.50f * PST_WINDOW_S);

            float p01 = sqrtf(sorted[idx_01]);
            float p1  = sqrtf(sorted[idx_1]);
            float p3  = sqrtf(sorted[idx_3]);
            float p10 = sqrtf(sorted[idx_10]);
            float p50 = sqrtf(sorted[idx_50]);

            fp->pst_10min = sqrtf(0.0314f * p01 + 0.0525f * p1 + 0.0657f * p3 +
                                  0.28f * p10 + 0.08f * p50);
        }
    }
}

/* ========================================================================
 * Get short-term Pst for a phase
 * ======================================================================== */
float flicker_get_pst(int phase) {
    if (phase < 0 || phase > 2) return 0.0f;
    return fph[phase].pst_short;
}

/* ========================================================================
 * Get long-term Plt (simplified: average of recent Pst values)
 * ======================================================================== */
float flicker_get_plt(void) {
    float sum = 0.0f;
    for (int p = 0; p < 3; p++) {
        sum += fph[p].pst_10min;
    }
    return sum / 3.0f;
}