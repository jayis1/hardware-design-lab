/*
 * dead_reckon.c — Inertial dead-reckoning for Inkwell stroke reconstruction
 *
 * Receives body-frame accelerometer readings and the current AHRS quaternion,
 * removes gravity by projecting the Earth-frame gravity vector back into the
 * body frame, and double-integrates the residual linear acceleration to
 * produce a pen-tip displacement delta in micrometers. Pen-lift events
 * (signaled by pressure_is_pen_down() going false) are zero-velocity
 * updates that snap velocity to zero and bound drift.
 *
 * Optical-flow deltas from the PMW3360 are fused into the position estimate
 * with a complementary filter: the optical flow dominates the low-frequency
 * position (drift) while the inertial estimate dominates the high-frequency
 * position (because the optical flow has ~1 ms latency). The fusion gain
 * alpha_of is tuned to 0.05 (5 % optical, 95 % inertial per 10 ms update).
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "dead_reckon.h"
#include "optflow.h"
#include "../board.h"
#include <math.h>

static float g_dt = 0.001f;
static float g_vx = 0.0f, g_vy = 0.0f, g_vz = 0.0f;  /* body-frame velocity, m/s */
static float g_dx_um = 0.0f, g_dy_um = 0.0f;         /* accumulated delta */
static float g_alpha_of = 0.05f;

#define GRAVITY_G   (9.80665f)

/* Rotate Earth-frame gravity (0,0,-1g) into body frame via q⁻¹ * g * q.
 * We use the closed form: a_g_body = R(q)^T * (0,0,-1g). */
static void gravity_body_frame(const float q[4], float gb[3])
{
    float q0 = q[0], q1 = q[1], q2 = q[2], q3 = q[3];
    /* R^T * (0,0,-1) gives the third column of R^T negated:
     * gb_x = -2*(q1*q3 - q0*q2)
     * gb_y = -2*(q2*q3 + q0*q1)
     * gb_z = -(1 - 2*(q1^2 + q2^2)) */
    gb[0] = -2.0f * (q1 * q3 - q0 * q2);
    gb[1] = -2.0f * (q2 * q3 + q0 * q1);
    gb[2] = -(1.0f - 2.0f * (q1 * q1 + q2 * q2));
}

void dead_reckon_init(uint32_t sample_hz)
{
    g_dt = 1.0f / (float)sample_hz;
    g_vx = g_vy = g_vz = 0.0f;
    g_dx_um = g_dy_um = 0.0f;
}

void dead_reckon_update(const float accel_g[3], const float q[4],
                        float dt_s, float a_lin_out[3])
{
    /* Remove gravity (in g units) from raw accelerometer. */
    float gb[3];
    gravity_body_frame(q, gb);
    float ax_lin_g = accel_g[0] - gb[0];
    float ay_lin_g = accel_g[1] - gb[1];
    float az_lin_g = accel_g[2] - gb[2];

    /* Convert to m/s^2. */
    float ax_lin = ax_lin_g * GRAVITY_G;
    float ay_lin = ay_lin_g * GRAVITY_G;
    float az_lin = az_lin_g * GRAVITY_G;

    if (a_lin_out) {
        a_lin_out[0] = ax_lin;
        a_lin_out[1] = ay_lin;
        a_lin_out[2] = az_lin;
    }

    /* Integrate velocity (body frame). */
    float dt = (dt_s > 0.0f) ? dt_s : g_dt;
    g_vx += ax_lin * dt;
    g_vy += ay_lin * dt;
    g_vz += az_lin * dt;

    /* Integrate position (body frame, x/y on the writing plane). */
    g_dx_um += g_vx * dt * 1.0e6f;
    g_dy_um += g_vy * dt * 1.0e6f;
}

void dead_reckon_zupt(void)
{
    /* Pen lifted: velocity is exactly zero. This is the key drift bound. */
    g_vx = 0.0f;
    g_vy = 0.0f;
    g_vz = 0.0f;
}

void dead_reckon_fuse_optflow(int16_t dx_counts, int16_t dy_counts, uint8_t squal)
{
    (void)squal;  /* caller already gated on SQUAL threshold */
    float um_per_count = 25400.0f / 1200.0f;  /* default CPI */
    /* Convert counts to µm. The optical flow y axis is flipped relative
     * to the IMU x; the mounting offset is handled here. */
    float of_dx_um = (float)dx_counts * um_per_count;
    float of_dy_um = -(float)dy_counts * um_per_count;

    /* Complementary fusion: blend optical-flow delta into inertial delta. */
    g_dx_um = g_dx_um * (1.0f - g_alpha_of) + of_dx_um * g_alpha_of;
    g_dy_um = g_dy_um * (1.0f - g_alpha_of) + of_dy_um * g_alpha_of;
}

void dead_reckon_get_delta(float *dx_um, float *dy_um)
{
    if (dx_um) *dx_um = g_dx_um;
    if (dy_um) *dy_um = g_dy_um;
}

void dead_reckon_clear_delta(void)
{
    g_dx_um = 0.0f;
    g_dy_um = 0.0f;
}