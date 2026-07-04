/*
 * trajectory.c — Dead-reckoning trajectory reconstruction + normalization
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * All math is done in single-precision float (Cortex-M4F FPU). Each stroke
 * lasts at most 1.5 s so integration drift is bounded; we additionally apply
 * a high-pass DC blocker and zero the velocity at stroke start.
 */
#include "trajectory.h"
#include "../board.h"
#include <math.h>

/* ---- Gravity-removing low-pass ------------------------------------ */
/* Exponential moving average with alpha = 0.02 → ~3 Hz cutoff at 1 kHz. */
static void remove_gravity(const imu_sample_t *s, uint16_t n,
                           float *ax_out, float *ay_out, float *az_out)
{
    float gx = 0.0f, gy = 0.0f, gz = 0.0f;
    const float alpha = 0.02f;
    const float accel_lsb_g = 4096.0f;   /* LSB per g at ±8 g */

    for (uint16_t i = 0; i < n; i++) {
        float ax = (float)s[i].ax / accel_lsb_g;
        float ay = (float)s[i].ay / accel_lsb_g;
        float az = (float)s[i].az / accel_lsb_g;
        gx = gx * (1.0f - alpha) + ax * alpha;
        gy = gy * (1.0f - alpha) + ay * alpha;
        gz = gz * (1.0f - alpha) + az * alpha;
        ax_out[i] = ax - gx;
        ay_out[i] = ay - gy;
        az_out[i] = az - gz;
    }
}

/* ---- High-pass DC blocker on a float buffer ----------------------- */
static void dc_block(const float *in, float *out, uint16_t n)
{
    /* y[i] = x[i] - x[i-1] + 0.995 * y[i-1]   (one-pole HPF, ~0.3 Hz) */
    if (n == 0) return;
    out[0] = 0.0f;
    for (uint16_t i = 1; i < n; i++) {
        out[i] = in[i] - in[i-1] + 0.995f * out[i-1];
    }
}

/* ---- Trajectory reconstruction ------------------------------------ */
int trajectory_reconstruct(const imu_sample_t *samples,
                           uint16_t count,
                           trajectory_t *out)
{
    if (count < STROKE_MIN_SAMP || count > STROKE_MAX_SAMP) return -1;

    /* Buffers on stack (count ≤ STROKE_MAX_SAMP = 1500 → 6 KB stack use). */
    static float ax[STROKE_MAX_SAMP], ay[STROKE_MAX_SAMP], az[STROKE_MAX_SAMP];
    static float hpx[STROKE_MAX_SAMP], hpy[STROKE_MAX_SAMP], hpz[STROKE_MAX_SAMP];
    static float vx[STROKE_MAX_SAMP], vy[STROKE_MAX_SAMP], vz[STROKE_MAX_SAMP];
    static float px[STROKE_MAX_SAMP], py[STROKE_MAX_SAMP], pz[STROKE_MAX_SAMP];

    remove_gravity(samples, count, ax, ay, az);

    /* High-pass each axis to kill slowly accumulating DC drift. */
    dc_block(ax, hpx, count);
    dc_block(ay, hpy, count);
    dc_block(az, hpz, count);

    /* Double-integrate to position with zero initial velocity. */
    const float dt = 1.0f / (float)SAMPLE_RATE_HZ;
    vx[0] = vy[0] = vz[0] = 0.0f;
    px[0] = py[0] = pz[0] = 0.0f;
    for (uint16_t i = 1; i < count; i++) {
        vx[i] = vx[i-1] + hpx[i] * dt;
        vy[i] = vy[i-1] + hpy[i] * dt;
        vz[i] = vz[i-1] + hpz[i] * dt;
        px[i] = px[i-1] + vx[i] * dt;
        py[i] = py[i-1] + vy[i] * dt;
        pz[i] = pz[i-1] + vz[i] * dt;
    }

    /* Estimate the average forearm plane normal over the stroke using
     * the gyro-integrated pitch. For a wrist-worn sensor, the writing plane
     * is roughly perpendicular to the forearm axis. We approximate the
     * forearm axis as the low-pass filtered gravity direction (which, when
     * the wrist is held level and writing, points downward and forward). */
    float nx = 0.0f, ny = 0.0f, nz = 0.0f;
    const float alpha_g = 0.005f;
    float lgx = 0.0f, lgy = 0.0f, lgz = 0.0f;
    const float accel_lsb_g = 4096.0f;
    for (uint16_t i = 0; i < count; i++) {
        float gx = (float)samples[i].ax / accel_lsb_g;
        float gy = (float)samples[i].ay / accel_lsb_g;
        float gz = (float)samples[i].az / accel_lsb_g;
        lgx = lgx * (1.0f - alpha_g) + gx * alpha_g;
        lgy = lgy * (1.0f - alpha_g) + gy * alpha_g;
        lgz = lgz * (1.0f - alpha_g) + gz * alpha_g;
    }
    /* Use the final low-passed gravity as the plane normal. */
    nx = lgx; ny = lgy; nz = lgz;
    float nlen = sqrtf(nx*nx + ny*ny + nz*nz);
    if (nlen < 1e-6f) { nx = 0.0f; ny = 0.0f; nz = 1.0f; nlen = 1.0f; }
    nx /= nlen; ny /= nlen; nz /= nlen;

    /* Project the 3D trajectory onto the plane by removing the component
     * along the normal. Then pick two orthonormal basis vectors u, w on the
     * plane to form a 2D coordinate. */
    /* u = (ny, -nx, 0) normalized; w = n × u */
    float ux = ny,  uy = -nx, uz = 0.0f;
    float ulen = sqrtf(ux*ux + uy*uy + uz*uz);
    if (ulen < 1e-6f) { ux = 1.0f; uy = 0.0f; uz = 0.0f; ulen = 1.0f; }
    ux /= ulen; uy /= ulen; uz /= ulen;
    /* w = n × u */
    float wx = ny*uz - nz*uy;
    float wy = nz*ux - nx*uz;
    float wz = nx*uy - ny*ux;

    /* Temporarily store the 2D trajectory in the trajectory_t struct as
     * raw float-equivalent int16 (Q15) — we'll overwrite after normalization. */
    (void)out;
    /* Compute 2D points (x2, y2) into temporary float arrays. */
    static float x2[STROKE_MAX_SAMP], y2[STROKE_MAX_SAMP];
    for (uint16_t i = 0; i < count; i++) {
        /* dot p with u and w */
        x2[i] = px[i]*ux + py[i]*uy + pz[i]*uz;
        y2[i] = px[i]*wx + py[i]*wy + pz[i]*wz;
    }

    /* Resample by arc length to TRAJ_NPOINTS. */
    static float cum[STROKE_MAX_SAMP];
    cum[0] = 0.0f;
    for (uint16_t i = 1; i < count; i++) {
        float dx = x2[i] - x2[i-1];
        float dy = y2[i] - y2[i-1];
        cum[i] = cum[i-1] + sqrtf(dx*dx + dy*dy);
    }
    float total = cum[count-1];
    if (total < 1e-6f) total = 1e-6f;

    static float rx[TRAJ_NPOINTS], ry[TRAJ_NPOINTS];
    rx[0] = x2[0]; ry[0] = y2[0];
    rx[TRAJ_NPOINTS-1] = x2[count-1]; ry[TRAJ_NPOINTS-1] = y2[count-1];
    for (int k = 1; k < TRAJ_NPOINTS - 1; k++) {
        float target = total * (float)k / (float)(TRAJ_NPOINTS - 1);
        /* Linear scan to find the segment containing `target`. */
        uint16_t j = 1;
        while (j < count && cum[j] < target) j++;
        if (j >= count) j = count - 1;
        float seg = cum[j] - cum[j-1];
        float t = (seg > 1e-6f) ? (target - cum[j-1]) / seg : 0.0f;
        rx[k] = x2[j-1] + (x2[j] - x2[j-1]) * t;
        ry[k] = y2[j-1] + (y2[j] - y2[j-1]) * t;
    }

    /* Center and scale. */
    float cx = 0.0f, cy = 0.0f;
    for (int k = 0; k < TRAJ_NPOINTS; k++) { cx += rx[k]; cy += ry[k]; }
    cx /= TRAJ_NPOINTS; cy /= TRAJ_NPOINTS;
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (int k = 0; k < TRAJ_NPOINTS; k++) {
        rx[k] -= cx; ry[k] -= cy;
        if (rx[k] < minx) minx = rx[k];
        if (rx[k] > maxx) maxx = rx[k];
        if (ry[k] < miny) miny = ry[k];
        if (ry[k] > maxy) maxy = ry[k];
    }
    float diag = sqrtf((maxx-minx)*(maxx-minx) + (maxy-miny)*(maxy-miny));
    if (diag < 1e-6f) diag = 1e-6f;

    /* Speed channel (point-to-point distance in normalized units). */
    for (int k = 0; k < TRAJ_NPOINTS; k++) {
        out->x[k] = (int16_t)((rx[k] / diag) * 32767.0f);
        out->y[k] = (int16_t)((ry[k] / diag) * 32767.0f);
    }
    out->v[0] = 0;
    for (int k = 1; k < TRAJ_NPOINTS; k++) {
        float dx = (rx[k] - rx[k-1]) / diag;
        float dy = (ry[k] - ry[k-1]) / diag;
        float sp = sqrtf(dx*dx + dy*dy) * 32767.0f;
        if (sp > 32767.0f) sp = 32767.0f;
        out->v[k] = (int16_t)sp;
    }
    return 0;
}

int trajectory_normalize(trajectory_t *t)
{
    /* Already normalized during reconstruction; this is a hook for
     * additional per-axis whitening if training data is uploaded. */
    (void)t;
    return 0;
}