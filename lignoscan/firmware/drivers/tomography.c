/*
 * tomography.c — SART Tomographic Reconstruction Implementation
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 *
 * This is the core algorithmic module of LignoScan. It implements:
 * 1. Ray tracing: computing which grid cells each ultrasonic ray
 *    passes through and the path length in each cell.
 * 2. SART (Simultaneous Algebraic Reconstruction Technique): an
 *    iterative algorithm that solves the inverse problem of
 *    reconstructing the 2D slowness (1/velocity) distribution
 *    from the set of measured travel times.
 * 3. Decay classification: converting the velocity map into
 *    color-coded categories (sound/moderate/severe/hollow).
 * 4. Tomographic Decay Index (TDI): a single metric for overall
 *    trunk cross-section integrity.
 *
 * The SART algorithm requires double-precision floating point for
 * numerical stability over many iterations. The STM32H733's hardware
 * DP-FPU is essential here.
 */

#include "tomography.h"
#include "board.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- Compute Euclidean chord length between two sensors on the perimeter ---- */
float tomo_chord_length(float radius, float angle1, float angle2) {
    /* Chord length = 2 * R * sin(|Δθ| / 2) */
    float delta = fabsf(angle1 - angle2);
    if (delta > M_PI) delta = 2.0f * (float)M_PI - delta;  /* Shorter arc */
    return 2.0f * radius * sinf(delta / 2.0f);
}

/* ---- Initialize tomography context ---- */
void tomo_init(tomo_ctx_t *ctx, int num_sensors, float *positions,
               float diameter_cm, int n_radial, int n_angular) {
    memset(ctx, 0, sizeof(tomo_ctx_t));

    ctx->num_sensors = num_sensors;
    ctx->trunk_radius_m = diameter_cm / 100.0f / 2.0f;  /* cm → m, diameter → radius */
    ctx->n_radial = n_radial;
    ctx->n_angular = n_angular;
    ctx->n_cells = n_radial * n_angular;

    /* Copy sensor positions */
    for (int i = 0; i < num_sensors && i < 16; i++) {
        ctx->sensor_pos[i] = positions[i];
    }

    /* Initialize slowness to a uniform estimate.
     * Average wood velocity ~2500 m/s → slowness = 1/2500 s/m
     * = 4e-4 s/m = 400 ns/m */
    float initial_slowness = 400.0f;  /* ns/m (will work in ns throughout) */
    for (int i = 0; i < ctx->n_cells; i++) {
        ctx->slowness[i] = initial_slowness;
    }

    ctx->num_rays = 0;
    ctx->iteration = 0;
}

/* ---- Add a ray (TX→RX pair with measured ToF) ---- */
void tomo_add_ray(tomo_ctx_t *ctx, int tx, int rx, float tof_ns) {
    if (ctx->num_rays >= MAX_RAYS) return;

    ray_t *ray = &ctx->rays[ctx->num_rays];
    ray->tx_idx = tx;
    ray->rx_idx = rx;
    ray->measured_tof_ns = tof_ns;
    ray->angle_start = ctx->sensor_pos[tx];
    ray->angle_end = ctx->sensor_pos[rx];
    ray->chord_length_m = tomo_chord_length(ctx->trunk_radius_m,
                                             ray->angle_start,
                                             ray->angle_end);
    ray->num_cells = 0;

    ctx->num_rays++;
}

/* ---- Convert polar grid (r, θ) to cell index ---- */
static int polar_to_cell(tomo_ctx_t *ctx, float r, float theta) {
    /* r: 0 to trunk_radius, theta: 0 to 2π */
    if (r < 0 || r >= ctx->trunk_radius_m) return -1;

    /* Normalize theta to [0, 2π) */
    while (theta < 0) theta += 2.0f * (float)M_PI;
    while (theta >= 2.0f * (float)M_PI) theta -= 2.0f * (float)M_PI;

    int r_idx = (int)((r / ctx->trunk_radius_m) * (float)ctx->n_radial);
    int a_idx = (int)((theta / (2.0f * (float)M_PI)) * (float)ctx->n_angular);

    if (r_idx >= ctx->n_radial) r_idx = ctx->n_radial - 1;
    if (a_idx >= ctx->n_angular) a_idx = ctx->n_angular - 1;

    return a_idx * ctx->n_radial + r_idx;
}

/* ---- Trace a single ray through the polar grid ---- */
/*
 * The ray is a straight chord from sensor_tx position on the perimeter
 * to sensor_rx position. We sample points along this chord and record
 * which cells the ray passes through and the approximate length in each.
 *
 * Sensor positions in Cartesian:
 *   tx: (R*cos(θ_tx), R*sin(θ_tx))
 *   rx: (R*cos(θ_rx), R*sin(θ_rx))
 *
 * We parameterize the chord as P(t) = tx + t*(rx-tx), t ∈ [0, 1]
 * and sample at fine intervals, accumulating path length per cell.
 */
void tomo_trace_ray(tomo_ctx_t *ctx, ray_t *ray) {
    float R = ctx->trunk_radius_m;
    float xt = R * cosf(ray->angle_start);
    float yt = R * sinf(ray->angle_start);
    float xr = R * cosf(ray->angle_end);
    float yr = R * sinf(ray->angle_end);

    /* Total chord length */
    float dx = xr - xt;
    float dy = yr - yt;
    float chord_len = sqrtf(dx * dx + dy * dy);
    ray->chord_length_m = chord_len;

    /* Number of samples along the ray: ~5 samples per cell diameter
     * Cell radial size = R / n_radial; we want 5 samples per that distance */
    float cell_size = R / (float)ctx->n_radial;
    int n_samples = (int)(chord_len / cell_size * 5.0f) + 2;
    if (n_samples > 500) n_samples = 500;
    if (n_samples < 10) n_samples = 10;

    float ds = chord_len / (float)n_samples;  /* Step length */

    /* Track which cells we've visited and accumulate lengths */
    int last_cell = -1;
    float current_cell_length = 0.0f;

    for (int i = 0; i <= n_samples; i++) {
        float t = (float)i / (float)n_samples;
        float px = xt + t * dx;
        float py = yt + t * dy;

        /* Convert to polar */
        float r = sqrtf(px * px + py * py);
        float theta = atan2f(py, px);
        if (theta < 0) theta += 2.0f * (float)M_PI;

        int cell = polar_to_cell(ctx, r, theta);

        if (cell == last_cell) {
            /* Same cell — accumulate length */
            current_cell_length += ds;
        } else {
            /* Cell changed — save previous cell */
            if (last_cell >= 0 && ray->num_cells < MAX_CELLS) {
                ray->cells[ray->num_cells] = last_cell;
                ray->cell_lengths[ray->num_cells] = current_cell_length;
                ray->num_cells++;
            }
            last_cell = cell;
            current_cell_length = ds;
        }
    }

    /* Save last cell segment */
    if (last_cell >= 0 && ray->num_cells < MAX_CELLS) {
        ray->cells[ray->num_cells] = last_cell;
        ray->cell_lengths[ray->num_cells] = current_cell_length;
        ray->num_cells++;
    }
}

/* ---- Trace all rays through the grid ---- */
void tomo_trace_rays(tomo_ctx_t *ctx) {
    for (int i = 0; i < ctx->num_rays; i++) {
        tomo_trace_ray(ctx, &ctx->rays[i]);
    }
}

/* ---- Single SART iteration ---- */
/*
 * SART update rule (simultaneous version):
 *
 * For each cell j:
 *   s_j^(k+1) = s_j^(k) + λ * Σ_i [ (t_measured_i - t_computed_i) * w_ij / W_j ]
 *
 * Where:
 *   t_computed_i = Σ_j (s_j * l_ij)  — computed travel time for ray i
 *   w_ij = l_ij / Σ_j(l_ij) — weight (ray length in cell j / total ray length)
 *   W_j = Σ_i w_ij — normalization (sum of weights for cell j)
 *   λ = relaxation parameter (0.1 - 0.3)
 *   s_j = slowness of cell j (ns/m)
 *   l_ij = length of ray i in cell j (m)
 *
 * The "simultaneous" variant computes all corrections first, then
 * applies them at once — more stable than ART (which updates per-ray).
 */
void tomo_sart_iterate(tomo_ctx_t *ctx) {
    /* Trace rays on first iteration */
    if (ctx->iteration == 0) {
        tomo_trace_rays(ctx);
    }

    float lambda = 0.2f;  /* Relaxation parameter */

    /* Accumulate corrections for each cell */
    float correction[MAX_CELLS];
    float weight_sum[MAX_CELLS];
    memset(correction, 0, sizeof(correction));
    memset(weight_sum, 0, sizeof(weight_sum));

    /* For each ray, compute residual and distribute to cells */
    for (int i = 0; i < ctx->num_rays; i++) {
        ray_t *ray = &ctx->rays[i];

        if (ray->num_cells == 0) continue;

        /* Compute predicted travel time: t_pred = Σ(s_j * l_ij) */
        double t_predicted = 0.0;
        double total_length = 0.0;

        for (int c = 0; c < ray->num_cells; c++) {
            int cell_idx = ray->cells[c];
            float length = ray->cell_lengths[c];
            t_predicted += (double)ctx->slowness[cell_idx] * (double)length;
            total_length += (double)length;
        }

        if (total_length < 1e-9) continue;

        /* Residual: Δt = t_measured - t_predicted */
        double residual = (double)ray->measured_tof_ns - t_predicted;

        /* Distribute residual to cells weighted by path length */
        for (int c = 0; c < ray->num_cells; c++) {
            int cell_idx = ray->cells[c];
            float weight = (float)((double)ray->cell_lengths[c] / total_length);

            correction[cell_idx] += (float)(residual * (double)weight * (double)ray->cell_lengths[c] / total_length);
            weight_sum[cell_idx] += weight * weight;
        }
    }

    /* Apply corrections simultaneously */
    for (int j = 0; j < ctx->n_cells; j++) {
        if (weight_sum[j] > 1e-9f) {
            float delta = lambda * correction[j] / weight_sum[j];
            ctx->slowness[j] += delta;

            /* Clamp slowness to physical range:
             * Air: ~333 ns/m (very fast, velocity = 3000 m/s in air → but wood is slower)
             * Actually, slowness = 1/velocity * 1e9 (ns/m)
             * Sound wood: v=4000 m/s → s=250 ns/m
             * Severe decay: v=500 m/s → s=2000 ns/m
             * Air/hollow: v=340 m/s → s=2941 ns/m
             * So range: 200-3000 ns/m */
            if (ctx->slowness[j] < 200.0f) ctx->slowness[j] = 200.0f;
            if (ctx->slowness[j] > 3000.0f) ctx->slowness[j] = 3000.0f;
        }
    }

    ctx->iteration++;
}

/* ---- Finalize reconstruction: convert slowness to velocity and classify ---- */
void tomo_finalize(tomo_ctx_t *ctx) {
    /* Convert slowness (ns/m) to velocity (m/s):
     * velocity = 1e9 / slowness_ns_per_m */
    for (int j = 0; j < ctx->n_cells; j++) {
        float velocity = 1.0e9f / ctx->slowness[j];

        /* Map cell to 2D array for BLE transmission.
         * Cell index j = a_idx * n_radial + r_idx
         * We store as velocity_map[angular][radial] */
        int a_idx = j / ctx->n_radial;
        int r_idx = j % ctx->n_radial;
        if (a_idx < 16 && r_idx < 16) {
            ctx->velocity_map[a_idx][r_idx] = velocity;
        }

        /* Classify decay */
        if (velocity > SOUND_WOOD_VMIN) {
            ctx->classification[j] = DECAY_SOUND;
        } else if (velocity > MOD_DECAY_VMIN) {
            ctx->classification[j] = DECAY_MODERATE;
        } else if (velocity > 500.0f) {
            ctx->classification[j] = DECAY_SEVERE;
        } else {
            ctx->classification[j] = DECAY_HOLLOW;
        }
    }
}

/* ---- Compute Tomographic Decay Index (TDI) ---- */
/*
 * TDI = (Area of non-sound cells) / (Total cross-section area)
 * Returns a value 0.0 (completely sound) to 1.0 (completely decayed).
 *
 * The area of each cell is proportional to r * dr * dθ (polar area element),
 * so inner cells weigh less than outer cells.
 */
float tomo_compute_tdi(tomo_ctx_t *ctx, float v_sound, float v_moderate) {
    (void)v_sound;
    (void)v_moderate;

    float total_area = 0.0f;
    float decayed_area = 0.0f;

    float R = ctx->trunk_radius_m;
    float dr = R / (float)ctx->n_radial;
    float dtheta = 2.0f * (float)M_PI / (float)ctx->n_angular;

    for (int j = 0; j < ctx->n_cells; j++) {
        int a_idx = j / ctx->n_radial;
        int r_idx = j % ctx->n_radial;

        /* Area of polar cell: (r_outer² - r_inner²) * dθ / 2 */
        float r_inner = (float)r_idx * dr;
        float r_outer = (float)(r_idx + 1) * dr;
        float cell_area = (r_outer * r_outer - r_inner * r_inner) * dtheta / 2.0f;

        total_area += cell_area;

        if (ctx->classification[j] != DECAY_SOUND) {
            decayed_area += cell_area;
        }
    }

    if (total_area < 1e-9f) return 0.0f;

    return decayed_area / total_area;
}

/* EOF — tomography.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */