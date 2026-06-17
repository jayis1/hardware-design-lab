/**
 * @file    tomography.c
 * @brief   SonicSight — Tomographic reconstruction engine.
 *          Implements straight-ray sensitivity matrix assembly,
 *          Tikhonov-regularised least-squares solver, and median
 *          post-filtering. Runs on-device using CMSIS-DSP.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <math.h>
#include "tomography.h"
#include "board.h"
#include "registers.h"

/* ========================================================================== */
/*  Initialisation                                                            */
/* ========================================================================== */

/* Private working buffers allocated in AXI SRAM (512 KB available) */
static float s_sensitivity_buf[TOMO_MAX_RAYS * TOMO_GRID_CELLS]; /* max 496×16384 = 8.1M — use subset */
static float s_image_buf[TOMO_GRID_CELLS];   /* max 16384 */
static float s_residual_buf[TOMO_MAX_RAYS];
static float s_temp_buf[TOMO_GRID_CELLS];    /* scratch */

void tomography_init(tomography_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(tomography_ctx_t));
    ctx->sensitivity = s_sensitivity_buf;
    ctx->image = s_image_buf;
    ctx->residual = s_residual_buf;
    ctx->lambda = TOMO_LAMBDA_DEFAULT;
    ctx->grid_size = TOMO_GRID_DEFAULT;
    ctx->model_diameter = 0.5f; /* default 50 cm */
}

/* ========================================================================== */
/*  Setup                                                                     */
/* ========================================================================== */

void tomography_setup(tomography_ctx_t *ctx,
                       const float *theta, const float *radius,
                       const uint8_t *active, uint8_t num_sensors,
                       const float (*tof_matrix)[TOMO_MAX_SENSORS],
                       uint32_t num_rays, uint32_t grid_size)
{
    for (uint8_t i = 0; i < num_sensors; i++) {
        ctx->sensor_theta[i]  = theta[i];
        ctx->sensor_radius[i] = radius[i];
        ctx->sensor_active[i] = active[i];
    }
    ctx->num_sensors = num_sensors;
    ctx->num_rays = num_rays;
    ctx->grid_size = (grid_size < TOMO_GRID_MIN) ? TOMO_GRID_MIN :
                     (grid_size > TOMO_GRID_MAX) ? TOMO_GRID_MAX : grid_size;
    ctx->grid_cell_size = ctx->model_diameter / (float)ctx->grid_size;

    /* Copy ToF matrix */
    for (uint8_t i = 0; i < num_sensors; i++) {
        for (uint8_t j = 0; j < num_sensors; j++) {
            ctx->tof_matrix[i][j] = tof_matrix[i][j];
        }
    }

    /* Build the sensitivity matrix */
    tomography_build_sensitivity(ctx);
}

/* ========================================================================== */
/*  Sensitivity Matrix Construction — Straight-Ray Tracing                    */
/* ========================================================================== */

int32_t tomography_build_sensitivity(tomography_ctx_t *ctx)
{
    uint32_t N = ctx->grid_size;
    uint32_t n_cells = N * N;
    float cell_size = ctx->grid_cell_size;
    float half_diam = ctx->model_diameter / 2.0f;

    /* Clear sensitivity matrix */
    memset(ctx->sensitivity, 0, TOMO_MAX_RAYS * TOMO_GRID_CELLS * sizeof(float));

    uint32_t ray_idx = 0;

    /* Iterate over all transmitter → receiver pairs */
    for (uint8_t tx = 0; tx < ctx->num_sensors; tx++) {
        if (!ctx->sensor_active[tx]) continue;

        for (uint8_t rx = tx + 1; rx < ctx->num_sensors; rx++) {
            if (!ctx->sensor_active[rx]) continue;
            if (ray_idx >= TOMO_MAX_RAYS) break;

            /* Convert sensor positions to cartesian coordinates */
            float tx_x = ctx->sensor_radius[tx] * cosf(ctx->sensor_theta[tx]);
            float tx_y = ctx->sensor_radius[tx] * sinf(ctx->sensor_theta[tx]);
            float rx_x = ctx->sensor_radius[rx] * cosf(ctx->sensor_theta[rx]);
            float rx_y = ctx->sensor_radius[rx] * sinf(ctx->sensor_theta[rx]);

            /* Ray direction vector */
            float dx = rx_x - tx_x;
            float dy = rx_y - tx_y;
            float ray_len = sqrtf(dx * dx + dy * dy);
            if (ray_len < 0.001f) continue;

            /* Normalise direction */
            dx /= ray_len;
            dy /= ray_len;

            /* Traverse ray: step through grid cells using digital differential analyser (DDA) */
            /* Start position (slightly inside the edge) */
            float px = tx_x + dx * 0.001f;
            float py = tx_y + dy * 0.001f;
            float remaining = ray_len;

            /* Clamp bounds to model diameter */
            while (remaining > 0.001f) {
                /* Convert position to grid coordinates (origin at centre) */
                float gx_f = (px + half_diam) / cell_size;
                float gy_f = (py + half_diam) / cell_size;

                int32_t gx = (int32_t)gx_f;
                int32_t gy = (int32_t)gy_f;

                if (gx >= 0 && gx < (int32_t)N && gy >= 0 && gy < (int32_t)N) {
                    /* Compute distances to next grid line in x and y */
                    float next_gx = (dx > 0) ? ((float)(gx + 1) * cell_size - half_diam)
                                              : ((float)gx * cell_size - half_diam);
                    float next_gy = (dy > 0) ? ((float)(gy + 1) * cell_size - half_diam)
                                              : ((float)gy * cell_size - half_diam);
                    float t_to_x = (next_gx - px) / dx;
                    float t_to_y = (next_gy - py) / dy;

                    float t_step = (t_to_x < t_to_y) ? t_to_x : t_to_y;
                    if (t_step < 0.001f) t_step = 0.001f;
                    if (t_step > remaining) t_step = remaining;

                    /* Accumulate path length in this cell */
                    ctx->sensitivity[ray_idx * n_cells + gy * N + gx] += t_step;

                    /* Advance */
                    px += dx * t_step;
                    py += dy * t_step;
                    remaining -= t_step;
                } else {
                    /* Outside model area — step to boundary */
                    float t_to_boundary_x = (dx > 0) ? (half_diam - px) / dx
                                                     : (-half_diam - px) / dx;
                    float t_to_boundary_y = (dy > 0) ? (half_diam - py) / dy
                                                     : (-half_diam - py) / dy;
                    float t_boundary = (t_to_boundary_x < t_to_boundary_y)
                                       ? t_to_boundary_x : t_to_boundary_y;
                    if (t_boundary < 0.001f) break;
                    px += dx * t_boundary;
                    py += dy * t_boundary;
                    remaining -= t_boundary;
                }
            }

            ray_idx++;
        }
    }

    ctx->num_rays = ray_idx;
    return ERR_OK;
}

/* ========================================================================== */
/*  Tikhonov-Regularised Least-Squares Solver                                 */
/* ========================================================================== */

int32_t tomography_solve(tomography_ctx_t *ctx)
{
    uint32_t N = ctx->grid_size;
    uint32_t n_cells = N * N;
    uint32_t M = ctx->num_rays;

    if (M == 0 || n_cells == 0) {
        return ERR_TOMO_SOLVER;
    }

    /* --- Step 1: Build the ToF vector t (M elements) --- */
    float *t = s_residual_buf;  /* reuse residual buffer for RHS */
    uint32_t ray_idx = 0;
    for (uint8_t tx = 0; tx < ctx->num_sensors; tx++) {
        if (!ctx->sensor_active[tx]) continue;
        for (uint8_t rx = tx + 1; rx < ctx->num_sensors; rx++) {
            if (!ctx->sensor_active[rx]) continue;
            if (ray_idx >= M) break;

            float tof = ctx->tof_matrix[tx][rx];
            if (tof > 0.0f && tof < 10000.0f) {
                /* Convert ToF from µs to seconds for solver consistency */
                t[ray_idx] = tof * 1e-6f;
            } else {
                t[ray_idx] = 0.001f; /* default ~1 ms for missing paths */
            }
            ray_idx++;
        }
    }

    /* --- Step 2: Compute S^T S (N² × N²) and S^T t (N²) using normal equations --- */
    /* For large N, storing S^T S explicitly is prohibitive.
     * We use the conjugate gradient (CG) method with implicit operator,
     * which only requires S·x and S^T·y products.
     * 
     * Here we implement a simplified CG solver.
     */

    /* Initial guess: homogeneous slowness (1/1500 m/s ≈ 0.000667 s/m) */
    float *m = ctx->image;
    for (uint32_t i = 0; i < n_cells; i++) {
        m[i] = 0.000667f;
    }

    /* --- Conjugate Gradient Iteration --- */
    float *r = s_temp_buf;         /* residual = S^T t − (S^T S + λI) m */
    float *p = ctx->sensitivity;   /* reuse sensitivity buf for conjugate direction */
    float *Ap = s_residual_buf;    /* A·p product */
    float rsold, rsnew, alpha, beta;

    /* Compute initial residual: r = S^T (t − S·m) − λ m */
    /* First compute S·m → Ap (temp) */
    memset(Ap, 0, M * sizeof(float));
    for (uint32_t ray = 0; ray < M; ray++) {
        float dot = 0.0f;
        for (uint32_t cell = 0; cell < n_cells; cell++) {
            dot += ctx->sensitivity[ray * n_cells + cell] * m[cell];
        }
        Ap[ray] = dot;
    }

    /* Compute S^T (t − S·m) − λ m */
    memset(r, 0, n_cells * sizeof(float));
    for (uint32_t cell = 0; cell < n_cells; cell++) {
        float st_res = 0.0f;
        for (uint32_t ray = 0; ray < M; ray++) {
            st_res += ctx->sensitivity[ray * n_cells + cell] * (t[ray] - Ap[ray]);
        }
        r[cell] = st_res - ctx->lambda * m[cell];
    }

    /* Initial conjugate direction p = r */
    memcpy(p, r, n_cells * sizeof(float));

    /* rsold = r^T r */
    rsold = 0.0f;
    for (uint32_t i = 0; i < n_cells; i++) {
        rsold += r[i] * r[i];
    }

    /* CG iterations */
    uint32_t max_iter = n_cells; /* typically converges in < n_cells iterations */
    for (uint32_t iter = 0; iter < max_iter && iter < 2000; iter++) {
        /* Compute Ap = (S^T S + λI) p */
        /* First compute S·p → temp vector */
        memset(Ap, 0, M * sizeof(float));
        for (uint32_t ray = 0; ray < M; ray++) {
            float dot = 0.0f;
            for (uint32_t cell = 0; cell < n_cells; cell++) {
                dot += ctx->sensitivity[ray * n_cells + cell] * p[cell];
            }
            Ap[ray] = dot;
        }

        /* Then Ap = S^T (S·p) + λ p */
        memset(Ap, 0, n_cells * sizeof(float)); /* reuse — now n_cells size */
        for (uint32_t cell = 0; cell < n_cells; cell++) {
            float sum = 0.0f;
            for (uint32_t ray = 0; ray < M; ray++) {
                sum += ctx->sensitivity[ray * n_cells + cell] * Ap[ray];
            }
            Ap[cell] = sum + ctx->lambda * p[cell];
        }

        /* α = rsold / (p^T Ap) */
        float pAp = 0.0f;
        for (uint32_t i = 0; i < n_cells; i++) {
            pAp += p[i] * Ap[i];
        }
        if (fabsf(pAp) < 1e-15f) break; /* numerical breakdown */
        alpha = rsold / pAp;

        /* m = m + α p */
        for (uint32_t i = 0; i < n_cells; i++) {
            m[i] += alpha * p[i];
        }

        /* r = r − α Ap */
        for (uint32_t i = 0; i < n_cells; i++) {
            r[i] -= alpha * Ap[i];
        }

        /* rsnew = r^T r */
        rsnew = 0.0f;
        for (uint32_t i = 0; i < n_cells; i++) {
            rsnew += r[i] * r[i];
        }

        /* Convergence check */
        if (sqrtf(rsnew) < 1e-6f) break;

        /* β = rsnew / rsold */
        beta = rsnew / rsold;

        /* p = r + β p */
        for (uint32_t i = 0; i < n_cells; i++) {
            p[i] = r[i] + beta * p[i];
        }

        rsold = rsnew;
    }

    /* --- Step 3: Enforce positivity (slowness must be > 0) --- */
    float min_slowness = 1.0f / 5000.0f; /* 5000 m/s max velocity */
    for (uint32_t i = 0; i < n_cells; i++) {
        if (m[i] < min_slowness) m[i] = min_slowness;
    }

    return ERR_OK;
}

/* ========================================================================== */
/*  Median Filter — Remove salt-and-pepper noise from reconstruction          */
/* ========================================================================== */

void tomography_median_filter(float *image, uint32_t width,
                               uint32_t height, uint32_t ksize)
{
    if (ksize < 3 || ksize > 7 || (ksize % 2) == 0) ksize = 3;
    int32_t half = (int32_t)ksize / 2;
    float *result = s_temp_buf;  /* use scratch buffer */

    uint32_t total = width * height;
    memcpy(result, image, total * sizeof(float));

    /* Simple median via insertion sort of kernel window */
    float window[49]; /* max 7×7 = 49 */
    for (int32_t y = 0; y < (int32_t)height; y++) {
        for (int32_t x = 0; x < (int32_t)width; x++) {
            uint32_t n = 0;
            for (int32_t ky = -half; ky <= half; ky++) {
                for (int32_t kx = -half; kx <= half; kx++) {
                    int32_t py = y + ky;
                    int32_t px = x + kx;
                    if (py >= 0 && py < (int32_t)height &&
                        px >= 0 && px < (int32_t)width) {
                        window[n++] = image[py * width + px];
                    }
                }
            }
            /* Insertion sort for median */
            if (n > 0) {
                for (uint32_t i = 1; i < n; i++) {
                    float key = window[i];
                    int32_t j = (int32_t)i - 1;
                    while (j >= 0 && window[j] > key) {
                        window[j + 1] = window[j];
                        j--;
                    }
                    window[j + 1] = key;
                }
                result[y * width + x] = window[n / 2];
            }
        }
    }

    /* Copy back */
    memcpy(image, result, total * sizeof(float));
}

/* ========================================================================== */
/*  Statistics Computation                                                    */
/* ========================================================================== */

void tomography_compute_stats(tomography_ctx_t *ctx)
{
    uint32_t n_cells = ctx->grid_size * ctx->grid_size;
    float vmin = 1e10f, vmax = 0.0f, vsum = 0.0f, vsum_sq = 0.0f;
    uint32_t valid_cells = 0;

    for (uint32_t i = 0; i < n_cells; i++) {
        float slowness = ctx->image[i];
        if (slowness > 0.0f) {
            float velocity = 1.0f / slowness;
            /* Reject unrealistic velocities (noise outside model) */
            if (velocity > 200.0f && velocity < 10000.0f) {
                if (velocity < vmin) vmin = velocity;
                if (velocity > vmax) vmax = velocity;
                vsum += velocity;
                vsum_sq += velocity * velocity;
                valid_cells++;
            }
        }
    }

    ctx->vel_min = (vmin < 1e9f) ? vmin : 0.0f;
    ctx->vel_max = vmax;
    ctx->vel_mean = (valid_cells > 0) ? (vsum / (float)valid_cells) : 0.0f;

    float variance = (vsum_sq / (float)valid_cells) - (ctx->vel_mean * ctx->vel_mean);
    ctx->vel_std = (variance > 0.0f) ? sqrtf(variance) : 0.0f;
}