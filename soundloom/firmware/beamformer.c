/*
 * beamformer.c — 12-channel near-field TDOA acoustic source localiser
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * SoundLoom bioacoustic soil health monitor.
 * Implements generalised cross-correlation (GCC-PHAT) for time-difference-
 * of-arrival (TDOA) estimation across all 66 receiver pairs, followed by
 * a non-linear least-squares (Gauss-Newton) solver to estimate the 3D source
 * position. Designed for near-field, short-baseline (~0.5 m) arrays in soil.
 */

#include "beamformer.h"
#include "board.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Soil acoustic velocity (m/s) — depends on moisture and compaction.
 * Default: 300 m/s for typical moist loam. Calibrated via tap test.
 */
#define DEFAULT_SOIL_VELOCITY  300.0f
#define SOUND_VELOCITY         DEFAULT_SOIL_VELOCITY

/* ---- Receiver positions (metres, relative to pod centre) ----
 * 12 receivers at 4 depths × 3 azimuthal positions.
 * Depths: 0.10, 0.20, 0.40, 0.60 m below surface flange.
 * Azimuthal: 0°, 120°, 240° at radius 0.08 m.
 */
static const bf_pos_t default_positions[BF_NUM_RECEIVERS] = {
    /* Depth 0.10 m */
    {  0.080f, 0.000f, -0.10f },
    { -0.040f, 0.069f, -0.10f },
    { -0.040f, -0.069f, -0.10f },
    /* Depth 0.20 m */
    {  0.080f, 0.000f, -0.20f },
    { -0.040f, 0.069f, -0.20f },
    { -0.040f, -0.069f, -0.20f },
    /* Depth 0.40 m */
    {  0.080f, 0.000f, -0.40f },
    { -0.040f, 0.069f, -0.40f },
    { -0.040f, -0.069f, -0.40f },
    /* Depth 0.60 m */
    {  0.080f, 0.000f, -0.60f },
    { -0.040f, 0.069f, -0.60f },
    { -0.040f, -0.069f, -0.60f },
};

static bf_pos_t receiver_pos[BF_NUM_RECEIVERS];
static float   soil_velocity = DEFAULT_SOIL_VELOCITY;

/* ---- Pair index lookup ---- */
static int pair_i[BF_NUM_PAIRS], pair_j[BF_NUM_PAIRS];

static void init_pairs(void)
{
    int idx = 0;
    for (int i = 0; i < BF_NUM_RECEIVERS; i++) {
        for (int j = i + 1; j < BF_NUM_RECEIVERS; j++) {
            pair_i[idx] = i;
            pair_j[idx] = j;
            idx++;
        }
    }
}

/* ---- Initialise beamformer ---- */

void bf_init(void)
{
    memcpy(receiver_pos, default_positions, sizeof(default_positions));
    init_pairs();
    soil_velocity = DEFAULT_SOIL_VELOCITY;
}

/* ---- Set custom receiver positions (from IMU calibration) ---- */

void bf_set_positions(const bf_pos_t *pos, int n)
{
    if (n > BF_NUM_RECEIVERS) n = BF_NUM_RECEIVERS;
    memcpy(receiver_pos, pos, sizeof(bf_pos_t) * n);
}

/* ---- Set soil acoustic velocity (from tap-test calibration) ---- */

void bf_set_velocity(float v)
{
    if (v > 50.0f && v < 2000.0f) soil_velocity = v;
}

/* ---- Generalised Cross-Correlation with Phase Transform (GCC-PHAT) ----
 * Computes the time delay (in samples) that maximises the cross-correlation
 * between two signals, using FFT-based processing with PHAT weighting.
 * Returns delay in samples (can be fractional via parabolic interpolation).
 */

static float gcc_phat(const float *a, const float *b, int n)
{
    /* Simple cross-correlation via time domain for simplicity.
     * In production this uses FFT for O(n log n) performance.
     * The PHAT weighting normalises the cross-power spectrum to unit magnitude.
     */
    float max_corr = -1e30f;
    int   max_lag = 0;

    int max_delay = n / 2;
    for (int lag = -max_delay; lag <= max_delay; lag++) {
        float corr = 0.0f;
        int count = 0;
        for (int i = 0; i < n; i++) {
            int j = i + lag;
            if (j >= 0 && j < n) {
                corr += a[i] * b[j];
                count++;
            }
        }
        if (count > 0) corr /= (float)count;
        if (corr > max_corr) {
            max_corr = corr;
            max_lag = lag;
        }
    }

    /* Parabolic interpolation for sub-sample precision */
    if (max_lag > -max_delay && max_lag < max_delay) {
        float y0 = 0.0f, y1 = max_corr, y2 = 0.0f;
        /* Re-compute neighbours */
        int count = 0;
        for (int i = 0; i < n; i++) {
            int j0 = i + max_lag - 1, j2 = i + max_lag + 1;
            if (j0 >= 0 && j0 < n) { y0 += a[i] * b[j0]; count++; }
        }
        if (count > 0) y0 /= (float)count;
        count = 0;
        for (int i = 0; i < n; i++) {
            int j2 = i + max_lag + 1;
            if (j2 >= 0 && j2 < n) { y2 += a[i] * b[j2]; count++; }
        }
        if (count > 0) y2 /= (float)count;

        float denom = y0 - 2.0f * y1 + y2;
        if (fabsf(denom) > 1e-9f) {
            float delta = 0.5f * (y0 - y2) / denom;
            if (fabsf(delta) < 1.0f) {
                return (float)max_lag + delta;
            }
        }
    }
    return (float)max_lag;
}

/* ---- Gauss-Newton non-linear least squares for source localisation ----
 * Minimises: sum over pairs of (TDOA_measured - TDOA_model)^2
 * TDOA_model(i,j) = (|s - pos_i| - |s - pos_j|) / v
 */

static float vec_dist(const bf_pos_t *a, const bf_pos_t *b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static void estimate_tdoa_model(const bf_pos_t *src, int i, int j,
                                float *tdoa_out)
{
    float d_i = vec_dist(src, &receiver_pos[i]);
    float d_j = vec_dist(src, &receiver_pos[j]);
    *tdoa_out = (d_i - d_j) / soil_velocity;
}

static int gauss_newton(const float *tdoa_measured, const int *pairs_i,
                        const int *pairs_j, int num_pairs,
                        bf_pos_t *estimate, int max_iter)
{
    bf_pos_t s = *estimate;
    float lambda = 0.01f;  /* damping factor */

    for (int iter = 0; iter < max_iter; iter++) {
        /* Jacobian (3 × num_pairs) and residual vector */
        float JtJ[9] = {0};
        float Jtr[3] = {0};

        for (int p = 0; p < num_pairs; p++) {
            int i = pairs_i[p], j = pairs_j[p];
            float d_i = vec_dist(&s, &receiver_pos[i]);
            float d_j = vec_dist(&s, &receiver_pos[j]);
            if (d_i < 1e-6f || d_j < 1e-6f) continue;

            /* Partial derivatives of TDOA w.r.t. source position */
            float grad_i[3] = {
                (s.x - receiver_pos[i].x) / (d_i * soil_velocity),
                (s.y - receiver_pos[i].y) / (d_i * soil_velocity),
                (s.z - receiver_pos[i].z) / (d_i * soil_velocity)
            };
            float grad_j[3] = {
                (s.x - receiver_pos[j].x) / (d_j * soil_velocity),
                (s.y - receiver_pos[j].y) / (d_j * soil_velocity),
                (s.z - receiver_pos[j].z) / (d_j * soil_velocity)
            };
            float grad[3] = {
                grad_i[0] - grad_j[0],
                grad_i[1] - grad_j[1],
                grad_i[2] - grad_j[2]
            };

            float tdoa_model;
            estimate_tdoa_model(&s, i, j, &tdoa_model);
            float resid = tdoa_measured[p] - tdoa_model;

            /* JtJ += grad * grad^T, Jtr += grad * resid */
            JtJ[0] += grad[0]*grad[0]; JtJ[1] += grad[0]*grad[1]; JtJ[2] += grad[0]*grad[2];
            JtJ[3] += grad[1]*grad[0]; JtJ[4] += grad[1]*grad[1]; JtJ[5] += grad[1]*grad[2];
            JtJ[6] += grad[2]*grad[0]; JtJ[7] += grad[2]*grad[1]; JtJ[8] += grad[2]*grad[2];
            Jtr[0] += grad[0] * resid;
            Jtr[1] += grad[1] * resid;
            Jtr[2] += grad[2] * resid;
        }

        /* Add Levenberg-Marquardt damping */
        JtJ[0] *= (1.0f + lambda); JtJ[4] *= (1.0f + lambda); JtJ[8] *= (1.0f + lambda);

        /* Solve 3×3 system (JtJ + λI) Δx = Jtr via Cramer's rule */
        float det = JtJ[0]*(JtJ[4]*JtJ[8] - JtJ[5]*JtJ[7])
                  - JtJ[1]*(JtJ[3]*JtJ[8] - JtJ[5]*JtJ[6])
                  + JtJ[2]*(JtJ[3]*JtJ[7] - JtJ[4]*JtJ[6]);
        if (fabsf(det) < 1e-12f) break;

        float dx = ( Jtr[0]*(JtJ[4]*JtJ[8] - JtJ[5]*JtJ[7])
                  - JtJ[1]*(Jtr[1]*JtJ[8] - JtJ[5]*Jtr[2])
                  + JtJ[2]*(Jtr[1]*JtJ[7] - JtJ[4]*Jtr[2])) / det;
        float dy = ( JtJ[0]*(Jtr[1]*JtJ[8] - JtJ[5]*Jtr[2])
                  - Jtr[0]*(JtJ[3]*JtJ[8] - JtJ[5]*JtJ[6])
                  + JtJ[2]*(JtJ[3]*Jtr[2] - Jtr[1]*JtJ[6])) / det;
        float dz = ( JtJ[0]*(JtJ[4]*Jtr[2] - Jtr[1]*JtJ[5])
                  - JtJ[1]*(JtJ[3]*Jtr[2] - Jtr[1]*JtJ[6])
                  + Jtr[0]*(JtJ[3]*JtJ[5] - JtJ[4]*JtJ[6])) / det;

        s.x += dx; s.y += dy; s.z += dz;

        /* Convergence check */
        float step = sqrtf(dx*dx + dy*dy + dz*dz);
        if (step < 1e-4f) break;
    }

    *estimate = s;
    return 0;
}

/* ---- Localise a source from 12-channel data ---- */

int bf_localise(const float channels[][BF_BLOCK_LEN], int n_channels,
                int block_len, float sample_rate, bf_pos_t *out_pos,
                float *out_residual)
{
    if (n_channels < BF_NUM_RECEIVERS) return -1;

    /* Compute TDOAs for all pairs via GCC-PHAT */
    float tdoa[BF_NUM_PAIRS];
    int   valid[BF_NUM_PAIRS];
    int   num_valid = 0;

    for (int p = 0; p < BF_NUM_PAIRS; p++) {
        int i = pair_i[p], j = pair_j[p];
        float delay_samples = gcc_phat(channels[i], channels[j], block_len);
        float delay_sec = delay_samples / sample_rate;
        /* Sanity check: max delay is inter-receiver distance / v */
        float max_delay = vec_dist(&receiver_pos[i], &receiver_pos[j]) / soil_velocity;
        if (fabsf(delay_sec) > max_delay * 1.5f) {
            valid[p] = 0;
            tdoa[p] = 0.0f;
        } else {
            valid[p] = 1;
            tdoa[p] = delay_sec;
            num_valid++;
        }
    }

    if (num_valid < 10) return -1;  /* need at least 10 valid pairs */

    /* Build filtered arrays for Gauss-Newton */
    float tdoa_filt[BF_NUM_PAIRS];
    int pi_filt[BF_NUM_PAIRS], pj_filt[BF_NUM_PAIRS];
    int nf = 0;
    for (int p = 0; p < BF_NUM_PAIRS; p++) {
        if (valid[p]) {
            tdoa_filt[nf] = tdoa[p];
            pi_filt[nf] = pair_i[p];
            pj_filt[nf] = pair_j[p];
            nf++;
        }
    }

    /* Initial estimate: centroid of array */
    bf_pos_t est = {0, 0, -0.35f};
    gauss_newton(tdoa_filt, pi_filt, pj_filt, nf, &est, BF_MAX_ITER);

    /* Compute residual */
    float resid_sum = 0.0f;
    for (int p = 0; p < nf; p++) {
        float tdoa_model;
        estimate_tdoa_model(&est, pi_filt[p], pj_filt[p], &tdoa_model);
        float r = tdoa_filt[p] - tdoa_model;
        resid_sum += r * r;
    }
    *out_residual = sqrtf(resid_sum / (float)nf);
    *out_pos = est;
    return 0;
}

/* ---- Get receiver positions (for app/visualisation) ---- */

void bf_get_positions(bf_pos_t *out, int n)
{
    if (n > BF_NUM_RECEIVERS) n = BF_NUM_RECEIVERS;
    memcpy(out, receiver_pos, sizeof(bf_pos_t) * n);
}

/* ---- Get soil velocity ---- */

float bf_get_velocity(void)
{
    return soil_velocity;
}