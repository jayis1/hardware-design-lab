/*
 * reconstruct.c — EIT Reconstruction Solver
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 *
 * Implements the regularized Gauss-Newton inverse solver for EIT
 * reconstruction.  Given a measurement vector of 208 complex impedances
 * (from the adjacent stimulation protocol), this module solves the
 * inverse problem to recover a 2D conductivity distribution on a 576-node
 * FEM mesh.
 *
 * The algorithm:
 *   1. Compute the forward model (predicted voltages) for current
 *      conductivity estimate.
 *   2. Compute the Jacobian (sensitivity matrix) J, where J[i][j] =
 *      dV_i / dσ_j  (change in measurement i w.r.t. conductivity in element j).
 *   3. Solve:  Δσ = (J^T J + λ^2 I)^-1  J^T  ΔV
 *      where ΔV = measured - predicted, λ is regularization parameter.
 *   4. Update: σ_{k+1} = σ_k + Δσ
 *   5. Repeat for K iterations (typically 3).
 *
 * The Jacobian is pre-computed at initialization for a unit conductivity
 * background and reused (the linearized approximation is valid for small
 * conductivity contrasts, which holds for soil-root systems).
 */

#include "reconstruct.h"
#include "board.h"
#include <string.h>
#include <math.h>

/* ---------------------------------------------------------------------
 * Static storage for the FEM mesh and Jacobian
 * 576 nodes, 1082 triangular elements, 208 measurements
 * Jacobian is 208 × 576 = 119808 floats = 479 KB
 * J^T J is 576 × 576 = 331776 floats = 1.3 MB — too large for DTCM.
 * We store J in SRAM and J^T J in a packed representation.
 * --------------------------------------------------------------------- */

/* FEM mesh: node coordinates and element connectivity (pre-computed) */
typedef struct {
    float x, y;     /* node coordinates (mm) */
} mesh_node_t;

typedef struct {
    uint16_t n[3];  /* node indices for the triangle */
} mesh_element_t;

/* Unit-conductivity forward model voltages (208 measurements) */
static float s_forward_reference[EIT_FRAME_SIZE];

/* Jacobian matrix: J[meas][elem] — stored as [EIT_FRAME_SIZE][EIT_MESH_ELEMENTS]
 * For memory efficiency we use a reduced mesh of 196 elements for the
 * Jacobian (the 576-node mesh is interpolated from this). */
#define JACOBIAN_ROWS    EIT_FRAME_SIZE        /* 208 */
#define JACOBIAN_COLS    196                    /* reduced mesh */
#define JACOBIAN_SIZE   (JACOBIAN_ROWS * JACOBIAN_COLS)

static float s_jacobian[JACOBIAN_SIZE];
static float s_conductivity[JACOBIAN_COLS];

/* J^T J (196 × 196) and its regularized inverse */
static float s_jtj[JACOBIAN_COLS * JACOBIAN_COLS];  /* 196*196 = 38416 floats = 154 KB */
static float s_jtj_inv[JACOBIAN_COLS * JACOBIAN_COLS];

/* Configuration */
static recon_config_t s_config = {
    .lambda = EIT_REG_LAMBDA_DEFAULT,
    .num_iterations = EIT_GN_ITERATIONS,
    .mesh_resolution = 1,
    .conductivity_min = 0.001f,  /* 1 mS/m (very dry soil) */
    .conductivity_max = 0.5f,    /* 500 mS/m (saturated soil) */
};

/* ---------------------------------------------------------------------
 * Pre-compute the FEM mesh and Jacobian at initialization.
 *
 * The mesh is a 2D circular domain (representing the soil cross-section
 * inside the electrode ring) with 16 boundary nodes at electrode positions
 * and a triangulated interior.  We use a pre-computed mesh table.
 *
 * The Jacobian for the adjacent protocol on a unit-conductivity disk is
 * computed via the sensitivity formula:
 *   J[i][j] = -∫_{element_j} ∇u_i · ∇v_i dA
 * where u_i is the potential for stimulation pattern i, v_i is the
 * adjoint field for measurement pattern i.
 *
 * For a homogeneous disk, the sensitivity has a known analytic form,
 * so we can pre-compute it without running a full FEM solver.
 * --------------------------------------------------------------------- */

/* Electrode angular positions (radians) */
static void compute_electrode_positions(float angles[EIT_NUM_ELECTRODES])
{
    for (int i = 0; i < EIT_NUM_ELECTRODES; i++)
        angles[i] = 2.0f * 3.14159265f * i / EIT_NUM_ELECTRODES;
}

/* Analytic sensitivity for adjacent protocol on unit disk
 * S_ij = (cos(θ_i) - cos(θ_{i+1})) * (cos(θ_j) - cos(θ_{j+1}))
 *        + (sin(θ_i) - sin(θ_{i+1})) * (sin(θ_j) - sin(θ_{j+1}))
 * (simplified — the full expression involves Green's functions) */
static float compute_sensitivity(int stim, int meas, float r, float theta)
{
    float angles[16];
    compute_electrode_positions(angles);

    /* Stimulation pair */
    float th_s1 = angles[stim % 16];
    float th_s2 = angles[(stim + 1) % 16];
    /* Measurement pair */
    float th_m1 = angles[(stim + meas + 2) % 16];
    float th_m2 = angles[(stim + meas + 3) % 16];

    /* Dipole strengths at the boundary */
    float sx = cosf(th_s2) - cosf(th_s1);
    float sy = sinf(th_s2) - sinf(th_s1);
    float mx = cosf(th_m2) - cosf(th_m1);
    float my = sinf(th_m2) - sinf(th_m1);

    /* Position factor (interior point at r, theta) */
    float px = r * cosf(theta);
    float py = r * sinf(theta);

    /* Sensitivity ~ dot product of field gradients at the point */
    float sens = (sx * mx + sy * my) * (1.0f - px * px - py * py) / 2.0f;
    return sens;
}

void reconstruct_init(void)
{
    /* Compute Jacobian for reduced mesh (196 elements arranged in
     * concentric rings: 16 inners, then 32, 48, 64, 36 outer = 196) */
    memset(s_jacobian, 0, sizeof(s_jacobian));
    memset(s_conductivity, 0, sizeof(s_conductivity));

    /* Generate mesh: 4 concentric rings of triangular elements */
    int elem_idx = 0;
    for (int ring = 0; ring < 4; ring++) {
        float r_inner = (float)ring / 4.0f;
        float r_outer = (float)(ring + 1) / 4.0f;
        int n_seg = 16 * (ring + 1);  /* more segments in outer rings */
        for (int s = 0; s < n_seg && elem_idx < JACOBIAN_COLS; s++) {
            float theta1 = 2.0f * 3.14159265f * s / n_seg;
            float theta2 = 2.0f * 3.14159265f * (s + 1) / n_seg;
            float r_mid = (r_inner + r_outer) / 2.0f;
            float theta_mid = (theta1 + theta2) / 2.0f;

            /* For each measurement, compute sensitivity */
            for (int stim = 0; stim < EIT_NUM_STIM; stim++) {
                for (int meas = 0; meas < EIT_NUM_MEAS_PER_STIM; meas++) {
                    int row = stim * EIT_NUM_MEAS_PER_STIM + meas;
                    s_jacobian[row * JACOBIAN_COLS + elem_idx] =
                        compute_sensitivity(stim, meas, r_mid, theta_mid);
                }
            }
            elem_idx++;
        }
    }

    /* Compute J^T J */
    for (int i = 0; i < JACOBIAN_COLS; i++) {
        for (int j = 0; j < JACOBIAN_COLS; j++) {
            float sum = 0.0f;
            for (int k = 0; k < JACOBIAN_ROWS; k++) {
                sum += s_jacobian[k * JACOBIAN_COLS + i]
                     * s_jacobian[k * JACOBIAN_COLS + j];
            }
            s_jtj[i * JACOBIAN_COLS + j] = sum;
        }
    }

    /* Regularize: J^T J + λ² I, then invert via Cholesky decomposition */
    float lambda_sq = s_config.lambda * s_config.lambda;
    for (int i = 0; i < JACOBIAN_COLS; i++)
        s_jtj[i * JACOBIAN_COLS + i] += lambda_sq;

    /* Cholesky decomposition of J^T J + λ² I  (symmetric positive definite) */
    /* L[i][j] such that L L^T = A */
    static float L[JACOBIAN_COLS * JACOBIAN_COLS];
    memset(L, 0, sizeof(L));

    for (int i = 0; i < JACOBIAN_COLS; i++) {
        for (int j = 0; j <= i; j++) {
            float sum = s_jtj[i * JACOBIAN_COLS + j];
            for (int k = 0; k < j; k++)
                sum -= L[i * JACOBIAN_COLS + k] * L[j * JACOBIAN_COLS + k];
            if (i == j) {
                if (sum <= 0.0f) sum = 1e-10f;  /* numerical safety */
                L[i * JACOBIAN_COLS + j] = sqrtf(sum);
            } else {
                L[i * JACOBIAN_COLS + j] = sum / L[j * JACOBIAN_COLS + j];
            }
        }
    }

    /* Invert via forward/backward substitution: A^-1 = (L L^T)^-1 = L^-T L^-1 */
    /* First compute L^-1 */
    static float L_inv[JACOBIAN_COLS * JACOBIAN_COLS];
    memset(L_inv, 0, sizeof(L_inv));

    for (int i = 0; i < JACOBIAN_COLS; i++) {
        L_inv[i * JACOBIAN_COLS + i] = 1.0f / L[i * JACOBIAN_COLS + i];
        for (int j = 0; j < i; j++) {
            float sum = 0.0f;
            for (int k = j; k < i; k++)
                sum += L[i * JACOBIAN_COLS + k] * L_inv[k * JACOBIAN_COLS + j];
            L_inv[i * JACOBIAN_COLS + j] = -sum / L[i * JACOBIAN_COLS + i];
        }
    }

    /* A^-1 = L^-T * L^-1  (since A = L L^T, A^-1 = (L^-1)^T L^-1) */
    for (int i = 0; i < JACOBIAN_COLS; i++) {
        for (int j = 0; j < JACOBIAN_COLS; j++) {
            float sum = 0.0f;
            for (int k = MAX(i, j); k < JACOBIAN_COLS; k++) {
                sum += L_inv[k * JACOBIAN_COLS + i] * L_inv[k * JACOBIAN_COLS + j];
            }
            s_jtj_inv[i * JACOBIAN_COLS + j] = sum;
        }
    }

    /* Initialize conductivity to uniform background (0.05 S/m typical soil) */
    for (int i = 0; i < JACOBIAN_COLS; i++)
        s_conductivity[i] = 0.05f;

    /* Compute reference forward solution */
    reconstruct_forward_solve(s_conductivity, s_forward_reference);
}

/* ---------------------------------------------------------------------
 * Forward solve: predict voltages from conductivity
 * V_pred = V_ref * (σ / σ_ref)  (linearized approximation)
 * --------------------------------------------------------------------- */

void reconstruct_forward_solve(const float *conductivity,
                                 float *predicted_voltages)
{
    /* For the linearized model, predicted voltages = J @ conductivity
     * (first-order approximation around uniform background) */
    for (int i = 0; i < JACOBIAN_ROWS; i++) {
        float sum = 0.0f;
        for (int j = 0; j < JACOBIAN_COLS; j++)
            sum += s_jacobian[i * JACOBIAN_COLS + j] * conductivity[j];
        predicted_voltages[i] = sum;
    }
}

/* ---------------------------------------------------------------------
 * Gauss-Newton reconstruction update
 * --------------------------------------------------------------------- */

void reconstruct_frame(eit_frame_t *frame)
{
    if (!frame) return;

    /* Extract measurement vector from frame (use magnitudes) */
    float delta_v[JACOBIAN_ROWS];
    float v_pred[JACOBIAN_ROWS];

    for (int iter = 0; iter < s_config.num_iterations; iter++) {
        /* Compute forward prediction for current conductivity */
        reconstruct_forward_solve(s_conductivity, v_pred);

        /* Compute residual ΔV = V_meas - V_pred */
        for (int i = 0; i < JACOBIAN_ROWS; i++) {
            float v_meas = (float)frame->meas[i].mag / 1e6f;  /* scale to mV */
            delta_v[i] = v_meas - v_pred[i];
        }

        /* Compute J^T ΔV */
        float jt_delta_v[JACOBIAN_COLS];
        for (int j = 0; j < JACOBIAN_COLS; j++) {
            float sum = 0.0f;
            for (int i = 0; i < JACOBIAN_ROWS; i++)
                sum += s_jacobian[i * JACOBIAN_COLS + j] * delta_v[i];
            jt_delta_v[j] = sum;
        }

        /* Compute Δσ = (J^T J + λ² I)^-1 J^T ΔV = s_jtj_inv @ jt_delta_v */
        float delta_sigma[JACOBIAN_COLS];
        for (int i = 0; i < JACOBIAN_COLS; i++) {
            float sum = 0.0f;
            for (int j = 0; j < JACOBIAN_COLS; j++)
                sum += s_jtj_inv[i * JACOBIAN_COLS + j] * jt_delta_v[j];
            delta_sigma[i] = sum;
        }

        /* Update conductivity estimate */
        for (int i = 0; i < JACOBIAN_COLS; i++) {
            s_conductivity[i] += delta_sigma[i];
            /* Clamp to physical bounds */
            if (s_conductivity[i] < s_config.conductivity_min)
                s_conductivity[i] = s_config.conductivity_min;
            if (s_conductivity[i] > s_config.conductivity_max)
                s_conductivity[i] = s_config.conductivity_max;
        }
    }

    /* Copy reconstructed conductivity to frame (interpolate to 576 nodes) */
    memset(frame->conductivity, 0, sizeof(frame->conductivity));
    for (int i = 0; i < JACOBIAN_COLS && i < EIT_MESH_NODES; i++) {
        frame->conductivity[i] = s_conductivity[i];
    }
    /* Fill remaining nodes by interpolation (simplified: copy nearest) */
    for (int i = JACOBIAN_COLS; i < EIT_MESH_NODES; i++) {
        frame->conductivity[i] = s_conductivity[i % JACOBIAN_COLS];
    }
}

/* ---------------------------------------------------------------------
 * Render reconstruction to a 2D image buffer (for display)
 * --------------------------------------------------------------------- */

int reconstruct_get_image(const eit_frame_t *frame,
                            float *image_buffer,
                            int width, int height)
{
    if (!frame || !image_buffer || width <= 0 || height <= 0)
        return -1;

    /* Map conductivity values to a 2D grid using polar coordinates.
     * The FEM mesh is a disk; we sample it on a Cartesian grid. */
    float cmin = s_config.conductivity_min;
    float cmax = s_config.conductivity_max;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            /* Normalize to [-1, 1] */
            float fx = (2.0f * x / (float)(width - 1)) - 1.0f;
            float fy = (2.0f * y / (float)(height - 1)) - 1.0f;
            float r = sqrtf(fx * fx + fy * fy);
            float theta = atan2f(fy, fx);
            if (theta < 0) theta += 2.0f * 3.14159265f;

            float val;
            if (r > 1.0f) {
                val = 0.0f;  /* outside disk */
            } else {
                /* Map (r, theta) to mesh element index */
                int ring = (int)(r * 4.0f);
                if (ring > 3) ring = 3;
                int n_seg = 16 * (ring + 1);
                int seg = (int)(theta / (2.0f * 3.14159265f) * n_seg) % n_seg;
                int idx = 0;
                for (int rr = 0; rr < ring; rr++)
                    idx += 16 * (rr + 1);
                idx += seg;
                if (idx < JACOBIAN_COLS)
                    val = (frame->conductivity[idx] - cmin) / (cmax - cmin);
                else
                    val = 0.0f;
                if (val < 0) val = 0;
                if (val > 1) val = 1;
            }
            image_buffer[y * width + x] = val;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------
 * Compute a scalar "root biomass index" from the reconstruction
 * This integrates conductivity above a threshold (root signature)
 * --------------------------------------------------------------------- */

float reconstruct_compute_biomass(const eit_frame_t *frame)
{
    if (!frame) return 0.0f;

    /* Root biomass index = sum of (conductivity - background) over all
     * elements where conductivity > background + threshold */
    float background = 0.05f;  /* typical soil background */
    float threshold = 0.02f;   /* root signature threshold */
    float biomass = 0.0f;

    for (int i = 0; i < JACOBIAN_COLS && i < EIT_MESH_NODES; i++) {
        float excess = frame->conductivity[i] - background;
        if (excess > threshold) {
            /* Area element ~ proportional to ring area (simplified) */
            float area = 3.14159265f / JACOBIAN_COLS;
            biomass += excess * area;
        }
    }
    return biomass;
}

/* ---------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------- */

void reconstruct_set_config(const recon_config_t *cfg)
{
    if (!cfg) return;
    memcpy(&s_config, cfg, sizeof(s_config));
}