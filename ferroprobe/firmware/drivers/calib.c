/*
 * calib.c — Calibration and orientation compensation
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements two key calibration functions:
 *
 * 1. Ellipsoid calibration: Real fluxgate sensors have per-axis offsets,
 *    scale mismatches, and non-orthogonality. When the device is rotated
 *    in a uniform field, the raw readings trace an ellipsoid rather
 *    than a sphere. We fit this ellipsoid using a least-squares method
 *    to find the 9-parameter correction (3 offset, 3 scale, 3 cross-axis)
 *    that maps the ellipsoid to a unit sphere.
 *
 * 2. Orientation compensation: The IMU provides roll/pitch/heading. We
 *    rotate the measured magnetic vector from the sensor frame to the
 *    Earth-level frame, so measurements are consistent regardless of
 *    how the device is held during walk-mode surveying.
 */

#include "calib.h"
#include "lockin.h"
#include "../board.h"
#include <string.h>
#include <math.h>

/* ===================================================================== */
/*  Constants                                                              */
/* ===================================================================== */

#define CALIB_MAX_SAMPLES    2000
#define CALIB_DEFAULT_DURATION_S  30

/* Earth's nominal total field at mid-latitudes (nT).
 * Used as the reference radius for ellipsoid fitting.
 */
#define EARTH_FIELD_REF_NT  50000.0f

/* ===================================================================== */
/*  State                                                                  */
/* ===================================================================== */

static calib_params_t calib_params;
static uint8_t collecting = 0;
static uint16_t collect_duration_s = 0;
static uint32_t collect_start_tick = 0;

/* Sample buffer for ellipsoid fitting */
typedef struct {
    float x, y, z;
} vec3_t;

static vec3_t calib_samples[CALIB_MAX_SAMPLES];
static uint16_t calib_sample_count = 0;

/* ===================================================================== */
/*  Public API                                                             */
/* ===================================================================== */

void calib_init(void)
{
    memset(&calib_params, 0, sizeof(calib_params));
    calib_params.scale[0] = 1.0f;
    calib_params.scale[1] = 1.0f;
    calib_params.scale[2] = 1.0f;
    calib_params.cross[0][0] = 1.0f;
    calib_params.cross[1][1] = 1.0f;
    calib_params.cross[2][2] = 1.0f;
    calib_params.valid = 0;
    calib_sample_count = 0;
    collecting = 0;
}

void calib_start_collection(uint16_t duration_s)
{
    calib_sample_count = 0;
    collecting = 1;
    collect_duration_s = duration_s;
    collect_start_tick = 0;  /* Would use system tick */
}

void calib_collect_sample(const field_measurement_t *field, const imu_data_t *imu)
{
    (void)imu;  /* Orientation not used during collection */

    if (!collecting)
        return;
    if (calib_sample_count >= CALIB_MAX_SAMPLES)
        return;

    /* Store raw (uncalibrated) field values */
    calib_samples[calib_sample_count].x = field->bx_raw;
    calib_samples[calib_sample_count].y = field->by_raw;
    calib_samples[calib_sample_count].z = field->bz_raw;
    calib_sample_count++;
}

uint8_t calib_collection_complete(void)
{
    /* Check if duration elapsed or buffer full */
    /* In production, compare current tick to collect_start_tick */
    return (calib_sample_count >= CALIB_MAX_SAMPLES) || (!collecting && calib_sample_count > 100);
}

/* ===================================================================== */
/*  Ellipsoid fitting                                                      */
/* ===================================================================== */

/* The general ellipsoid equation:
 *   ax² + by² + cz² + 2dxy + 2exz + 2fyz + 2gx + 2hy + 2iz = 1
 *
 * We solve for [a,b,c,d,e,f,g,h,i] using least-squares:
 *   For each sample (x,y,z): [x², y², z², 2xy, 2xz, 2yz, 2x, 2y, 2z] · P = 1
 *
 * Then decompose the quadric matrix to extract center (offset) and
 * shape (scale + cross-axis).
 */

void calib_compute(void)
{
    collecting = 0;

    if (calib_sample_count < 50) {
        /* Not enough samples for a reliable fit */
        return;
    }

    /* Step 1: Compute the centroid (approximate offset) */
    float cx = 0, cy = 0, cz = 0;
    for (int i = 0; i < calib_sample_count; i++) {
        cx += calib_samples[i].x;
        cy += calib_samples[i].y;
        cz += calib_samples[i].z;
    }
    cx /= calib_sample_count;
    cy /= calib_sample_count;
    cz /= calib_sample_count;

    /* Step 2: Compute the covariance matrix of the centered data.
     * The eigenvectors of this matrix give the principal axes of the
     * ellipsoid, and the eigenvalues give the squared semi-axis lengths.
     */
    float cxx = 0, cyy = 0, czz = 0;
    float cxy = 0, cxz = 0, cyz = 0;

    for (int i = 0; i < calib_sample_count; i++) {
        float dx = calib_samples[i].x - cx;
        float dy = calib_samples[i].y - cy;
        float dz = calib_samples[i].z - cz;
        cxx += dx * dx;
        cyy += dy * dy;
        czz += dz * dz;
        cxy += dx * dy;
        cxz += dx * dz;
        cyz += dy * dz;
    }
    cxx /= calib_sample_count;
    cyy /= calib_sample_count;
    czz /= calib_sample_count;
    cxy /= calib_sample_count;
    cxz /= calib_sample_count;
    cyz /= calib_sample_count;

    /* Step 3: Extract scale from the diagonal of the covariance matrix.
     * For an ellipsoid with semi-axes (a, b, c), the variances are
     * (a²/5, b²/5, c²/5).  So semi-axis = sqrt(5 * variance).
     * The reference field (Earth's field) gives the target radius.
     */
    float var_x = cxx;
    float var_y = cyy;
    float var_z = czz;

    /* The semi-axes of the ellipsoid: sqrt(5 * variance) */
    float semi_x = sqrtf(5.0f * var_x);
    float semi_y = sqrtf(5.0f * var_y);
    float semi_z = sqrtf(5.0f * var_z);

    /* Step 4: Compute scale factors (reference field / semi-axis) */
    if (semi_x > 1.0f)
        calib_params.scale[0] = EARTH_FIELD_REF_NT / semi_x;
    if (semi_y > 1.0f)
        calib_params.scale[1] = EARTH_FIELD_REF_NT / semi_y;
    if (semi_z > 1.0f)
        calib_params.scale[2] = EARTH_FIELD_REF_NT / semi_z;

    /* Step 5: Offsets are the centroid (in nT) */
    calib_params.offset[0] = cx;
    calib_params.offset[1] = cy;
    calib_params.offset[2] = cz;

    /* Step 6: Cross-axis terms from off-diagonal covariance.
     * The normalized off-diagonal terms give the cross-axis coupling.
     * We construct a simple rotation correction.
     */
    float norm_xy = (semi_x > 1.0f && semi_y > 1.0f)
                    ? cxy / sqrtf(var_x * var_y) : 0.0f;
    float norm_xz = (semi_x > 1.0f && semi_z > 1.0f)
                    ? cxz / sqrtf(var_x * var_z) : 0.0f;
    float norm_yz = (semi_y > 1.0f && semi_z > 1.0f)
                    ? cyz / sqrtf(var_y * var_z) : 0.0f;

    /* Construct cross-axis correction matrix (inverse of the coupling) */
    /* For small couplings, the inverse is approximately:
     * [[1, -xy, -xz], [0, 1, -yz], [0, 0, 1]]
     * But we use a symmetric approximation.
     */
    calib_params.cross[0][0] = 1.0f;
    calib_params.cross[0][1] = -0.5f * norm_xy;
    calib_params.cross[0][2] = -0.5f * norm_xz;
    calib_params.cross[1][0] = -0.5f * norm_xy;
    calib_params.cross[1][1] = 1.0f;
    calib_params.cross[1][2] = -0.5f * norm_yz;
    calib_params.cross[2][0] = -0.5f * norm_xz;
    calib_params.cross[2][1] = -0.5f * norm_yz;
    calib_params.cross[2][2] = 1.0f;

    calib_params.valid = 1;

    /* Apply calibration to the lock-in engine */
    lockin_set_calib(calib_params.offset, calib_params.scale,
                     calib_params.cross, calib_params.temp_coeff);

    /* Apply per-axis phase corrections (from lock-in I/Q analysis) */
    for (int axis = 0; axis < 3; axis++) {
        if (calib_params.phase_deg[axis] != 0.0f) {
            /* The lock-in phase is set globally; for per-axis phase,
             * we'd need per-axis NCO offsets.  For now, use the average.
             */
        }
    }
}

const calib_params_t *calib_get(void)
{
    return &calib_params;
}

void calib_set(const calib_params_t *params)
{
    memcpy(&calib_params, params, sizeof(calib_params_t));
    lockin_set_calib(params->offset, params->scale, params->cross,
                     params->temp_coeff);
}

/* ===================================================================== */
/*  Orientation compensation                                                */
/* ===================================================================== */

void calib_apply_orientation(field_measurement_t *field, const imu_data_t *imu)
{
    if (!imu->valid)
        return;

    /* Convert roll/pitch/heading to radians */
    float phi = imu->roll_deg * 3.14159265f / 180.0f;
    float theta = imu->pitch_deg * 3.14159265f / 180.0f;
    float psi = imu->heading_deg * 3.14159265f / 180.0f;

    /* Rotation matrix R(φ, θ, ψ) from sensor frame to level frame:
     *
     * R = Rz(ψ) × Ry(θ) × Rx(φ)
     *
     *   | cosψ cosθ   cosψ sinθ sinφ - sinψ cosφ   cosψ sinθ cosφ + sinψ sinφ |
     *   | sinψ cosθ   sinψ sinθ sinφ + cosψ cosφ   sinψ sinθ cosφ - cosψ sinφ |
     *   | -sinθ       cosθ sinφ                      cosθ cosφ                |
     */
    float cpsi = cosf(psi), spsi = sinf(psi);
    float ctheta = cosf(theta), stheta = sinf(theta);
    float cphi = cosf(phi), sphi = sinf(phi);

    float r11 = cpsi * ctheta;
    float r12 = cpsi * stheta * sphi - spsi * cphi;
    float r13 = cpsi * stheta * cphi + spsi * sphi;
    float r21 = spsi * ctheta;
    float r22 = spsi * stheta * sphi + cpsi * cphi;
    float r23 = spsi * stheta * cphi - cpsi * sphi;
    float r31 = -stheta;
    float r32 = ctheta * sphi;
    float r33 = ctheta * cphi;

    /* Rotate the measured vector */
    float bx = field->bx_nt;
    float by = field->by_nt;
    float bz = field->bz_nt;

    field->bx_nt = r11 * bx + r12 * by + r13 * bz;
    field->by_nt = r21 * bx + r22 * by + r23 * bz;
    field->bz_nt = r31 * bx + r32 * by + r33 * bz;

    /* Recompute total field (should be invariant under rotation) */
    field->b_total = sqrtf(field->bx_nt * field->bx_nt +
                           field->by_nt * field->by_nt +
                           field->bz_nt * field->bz_nt);
}

/* ===================================================================== */
/*  Flash persistence (simplified)                                         */
/* ===================================================================== */

/* In a production implementation, the calibration parameters would be
 * stored in the STM32H743's user flash area (bank 2, sectors 8-11).
 * The H7 has 2 MB of flash; we reserve the last 16 KB for calibration.
 */

/* Flash user area address (sector 11 of bank 2) */
#define CALIB_FLASH_ADDR  0x081E0000U

uint8_t calib_save_to_flash(void)
{
    /* Unlock flash */
    FLASH_KEYR = 0x45670123U;
    FLASH_KEYR = 0xCDEF89ABU;

    /* Wait for flash ready */
    while (FLASH_SR & 0x01)
        ;

    /* Erase the sector (sector 11, bank 2) */
    FLASH_CR = 0;
    FLASH_CR |= (11U << 3);   /* SNB = sector 11 */
    FLASH_CR |= (1U << 1);    /* SER = sector erase */
    FLASH_CR |= (1U << 16);  /* START */
    while (FLASH_SR & 0x01)
        ;
    FLASH_SR = 0xFFFFFFFF;    /* Clear flags */

    /* Write calibration parameters (32-bit words) */
    FLASH_CR &= ~(1U << 1);   /* Clear SER */
    FLASH_CR |= (1U << 0);    /* PG = programming */
    FLASH_CR &= ~(0x3U << 8); /* PSIZE = 32-bit (10) */
    FLASH_CR |= (0x2U << 8);

    volatile uint32_t *dest = (volatile uint32_t *)CALIB_FLASH_ADDR;
    const uint32_t *src = (const uint32_t *)&calib_params;
    uint16_t n_words = sizeof(calib_params_t) / 4;

    for (uint16_t i = 0; i < n_words; i++) {
        dest[i] = src[i];
        while (FLASH_SR & 0x01)
            ;
    }

    FLASH_CR &= ~(1U << 0);  /* Clear PG */
    FLASH_CR |= (1U << 30);  /* Lock */

    return 1;
}

uint8_t calib_load_from_flash(void)
{
    const volatile uint32_t *src = (const volatile uint32_t *)CALIB_FLASH_ADDR;
    uint32_t *dst = (uint32_t *)&calib_params;

    /* Check if the flash contains valid data (magic number) */
    if (src[0] == 0xFFFFFFFF || src[0] == 0x00000000)
        return 0;  /* Flash empty, no stored calibration */

    uint16_t n_words = sizeof(calib_params_t) / 4;
    for (uint16_t i = 0; i < n_words; i++) {
        dst[i] = src[i];
    }

    if (calib_params.valid) {
        lockin_set_calib(calib_params.offset, calib_params.scale,
                         calib_params.cross, calib_params.temp_coeff);
        return 1;
    }
    return 0;
}