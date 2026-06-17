/**
 * @file    tomography.h
 * @brief   SonicSight — Tomographic reconstruction engine.
 *          Straight-ray Tikhonov-regularised least-squares solver
 *          for 2D acoustic velocity/slowness imaging.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TOMOGRAPHY_H
#define TOMOGRAPHY_H

#include <stdint.h>
#include "board.h"

/** Maximum grid dimension (128×128 = 16384 cells) */
#define TOMO_MAX_GRID      (128U)
/** Maximum number of rays (for N=32: 496) */
#define TOMO_GRID_CELLS    (TOMO_MAX_GRID * TOMO_MAX_GRID)

/** Tomography solver context */
typedef struct {
    /* Sensor geometry */
    float sensor_theta[TOMO_MAX_SENSORS];     /**< Sensor angles (radians) */
    float sensor_radius[TOMO_MAX_SENSORS];    /**< Sensor radii (metres) */
    uint8_t sensor_active[TOMO_MAX_SENSORS];  /**< Active sensor bitmap */
    uint8_t num_sensors;                      /**< Number of active sensors */

    /* Ray data */
    float tof_matrix[TOMO_MAX_SENSORS][TOMO_MAX_SENSORS]; /**< Time-of-flight (µs) */
    uint32_t num_rays;          /**< Total number of ray paths */

    /* Grid parameters */
    uint32_t grid_size;         /**< Grid dimension (N × N) */
    float grid_cell_size;       /**< Physical size of one cell (m) */
    float model_diameter;       /**< Reconstructed area diameter (m) */

    /* Regularisation */
    float lambda;               /**< Tikhonov regularisation parameter */
    float lambda_auto;          /**< Auto-selected λ (L-curve) */

    /* Solver state */
    float *sensitivity;         /**< Sensitivity matrix S (M × N²) — row-major */
    float *image;               /**< Output slowness image (N² elements) */
    float *residual;            /**< Residual vector (M elements) */

    /* Statistics */
    float vel_min;              /**< Minimum velocity in image (m/s) */
    float vel_max;              /**< Maximum velocity in image (m/s) */
    float vel_mean;             /**< Mean velocity (m/s) */
    float vel_std;              /**< Standard deviation of velocity */
} tomography_ctx_t;

/**
 * @brief Initialise tomography context.
 * @param ctx Tomography context (uninitialised).
 */
void tomography_init(tomography_ctx_t *ctx);

/**
 * @brief Setup geometry, grid, and ray data for reconstruction.
 * @param ctx           Tomography context.
 * @param theta         Sensor angles (radians).
 * @param radius        Sensor radii (metres).
 * @param active        Active sensor bitmap.
 * @param num_sensors   Number of sensors.
 * @param tof_matrix    Time-of-flight matrix (µs).
 * @param num_rays      Number of ray paths.
 * @param grid_size     Reconstruction grid dimension.
 */
void tomography_setup(tomography_ctx_t *ctx,
                       const float *theta, const float *radius,
                       const uint8_t *active, uint8_t num_sensors,
                       const float (*tof_matrix)[TOMO_MAX_SENSORS],
                       uint32_t num_rays, uint32_t grid_size);

/**
 * @brief Build sensitivity matrix S (ray tracing).
 *
 * For each ray (i→j), computes the path length through each grid cell
 * using a straight-ray Bresenham-like traversal.
 *
 * @param ctx Tomography context (must be setup first).
 * @return ERR_OK on success, negative on error.
 */
int32_t tomography_build_sensitivity(tomography_ctx_t *ctx);

/**
 * @brief Run Tikhonov-regularised least-squares inversion.
 *        Solves: (S^T S + λ diag(S^T S)) m = S^T t
 *
 * @param ctx Tomography context with built sensitivity matrix and ToF data.
 * @return ERR_OK on success, ERR_TOMO_SOLVER on failure.
 */
int32_t tomography_solve(tomography_ctx_t *ctx);

/**
 * @brief Apply median filter to the reconstructed image.
 * @param image     Image buffer (row-major).
 * @param width     Image width.
 * @param height    Image height.
 * @param ksize     Filter kernel size (3, 5, or 7).
 */
void tomography_median_filter(float *image, uint32_t width,
                               uint32_t height, uint32_t ksize);

/**
 * @brief Compute velocity statistics from the slowness image.
 * @param ctx Tomography context with solved image.
 */
void tomography_compute_stats(tomography_ctx_t *ctx);

#endif /* TOMOGRAPHY_H */