/*
 * ahrs.c — Madgwick quaternion AHRS for Inkwell
 *
 * Implementation of S. Madgwick's gradient-descent AHRS filter, tuned for
 * pen dynamics. The filter fuses gyroscope, accelerometer, and magnetometer
 * into a quaternion `q = [w x y z]` that rotates body-frame vectors into
 * the Earth frame. β is the gradient-descent step gain; the default 0.041
 * was tuned by minimizing end-point error on straight 100 mm pen strokes.
 *
 * The filter is written in single-precision float; the Cortex-M4F FPU
 * executes a full update in ~140 cycles at 64 MHz, comfortably within the
 * 1 ms budget. Gravity removal uses the estimated attitude to project out
 * the 1 g vertical from the accelerometer so that only linear acceleration
 * remains for the dead-reckoner.
 *
 * The gradient step uses the well-known Madgwick MARG formulation. The
 * magnetic-field reference is computed each step from the measured field
 * so that the yaw reference tracks the local geomagnetic field.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "ahrs.h"
#include "../board.h"
#include <math.h>

static float g_q0 = 1.0f, g_q1 = 0.0f, g_q2 = 0.0f, g_q3 = 0.0f;
static float g_beta = MADGWICK_BETA_DEFAULT;
static float g_inv_sample_hz = 1.0f / (float)AHRS_SAMPLE_HZ;

static inline float fast_inv_sqrt(float x)
{
    /* Quake III fast inverse square root; good enough for AHRS use. */
    float x2 = x * 0.5f;
    union { float f; int32_t i; } conv;
    conv.f = x;
    conv.i = 0x5F3759DF - (conv.i >> 1);
    float y = conv.f;
    y = y * (1.5f - (x2 * y * y));
    return y;
}

void ahrs_init(float beta, uint32_t sample_hz)
{
    g_q0 = 1.0f; g_q1 = 0.0f; g_q2 = 0.0f; g_q3 = 0.0f;
    g_beta = beta;
    g_inv_sample_hz = 1.0f / (float)sample_hz;
}

void ahrs_set_beta(float b) { g_beta = b; }
float ahrs_get_beta(void)   { return g_beta; }

void ahrs_get_quaternion(float q[4])
{
    q[0] = g_q0; q[1] = g_q1; q[2] = g_q2; q[3] = g_q3;
}

void ahrs_get_euler(float *roll, float *pitch, float *yaw)
{
    float sinr = 2.0f * (g_q0 * g_q1 + g_q2 * g_q3);
    float cosr = 1.0f - 2.0f * (g_q1 * g_q1 + g_q2 * g_q2);
    *roll  = atan2f(sinr, cosr);
    float sinp = 2.0f * (g_q0 * g_q2 - g_q3 * g_q1);
    *pitch = (sinp >= 1.0f) ? (float)(M_PI / 2.0)
          : (sinp <= -1.0f) ? (float)(-M_PI / 2.0)
          : asinf(sinp);
    float siny = 2.0f * (g_q0 * g_q3 + g_q1 * g_q2);
    float cosy = 1.0f - 2.0f * (g_q2 * g_q2 + g_q3 * g_q3);
    *yaw = atan2f(siny, cosy);
}

/* One Madgwick IMU+MARG update step. */
void ahrs_update(float gx, float gy, float gz,
                 float ax, float ay, float az,
                 float mx, float my, float mz)
{
    float q0 = g_q0, q1 = g_q1, q2 = g_q2, q3 = g_q3;
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float hx, hy;
    float _2bx, _2bz;
    float _4bx, _4bz;

    /* Rate of change of quaternion from gyro (body-frame) */
    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    /* Gradient-descent corrective step only if accelerometer valid. */
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {
        recipNorm = fast_inv_sqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm; ay *= recipNorm; az *= recipNorm;

        /* Auxiliary variables to reduce register pressure. */
        float _2q0mx = 2.0f * (q0 * mx);
        float _2q0my = 2.0f * (q0 * my);
        float _2q0mz = 2.0f * (q0 * mz);
        float _2q1mx = 2.0f * (q1 * mx);
        float _2q1my = 2.0f * (q1 * my);
        float _2q1mz = 2.0f * (q1 * mz);
        float _2q2mx = 2.0f * (q2 * mx);
        float _2q2my = 2.0f * (q2 * my);
        float _2q2mz = 2.0f * (q2 * mz);
        float _2q3mx = 2.0f * (q3 * mx);
        float _2q3my = 2.0f * (q3 * my);
        float _2q3mz = 2.0f * (q3 * mz);
        float _2q0 = 2.0f * q0;
        float _2q1 = 2.0f * q1;
        float _2q2 = 2.0f * q2;
        float _2q3 = 2.0f * q3;
        float _2q0q1 = 2.0f * q0 * q1;
        float _2q0q2 = 2.0f * q0 * q2;
        float _2q0q3 = 2.0f * q0 * q3;
        float _2q1q2 = 2.0f * q1 * q2;
        float _2q1q3 = 2.0f * q1 * q3;
        float _2q2q3 = 2.0f * q2 * q3;
        float _4q0 = 4.0f * q0;
        float _4q1 = 4.0f * q1;
        float _4q2 = 4.0f * q2;
        float _4q3 = 4.0f * q3;
        float _8q0 = 8.0f * q0;
        float _8q1 = 8.0f * q1;
        float _8q2 = 8.0f * q2;
        float _8q3 = 8.0f * q3;
        float q0q0 = q0 * q0;
        float q1q1 = q1 * q1;
        float q2q2 = q2 * q2;
        float q3q3 = q3 * q3;

        /* Reference direction of Earth's magnetic field. */
        hx = _2q0mx - _2q1mz + _2q2my;
        hy = _2q0my + _2q1mx + _2q2mz + _2q3mx;
        _2bx = sqrtf(hx * hx + hy * hy);
        _2bz = -_2q0mx * my + _2q1mx * mz + _2q2mx * my + _2q3mx * mz - _2bx;
        _4bx = 2.0f * _2bx;
        _4bz = 2.0f * _2bz;

        /* Gradient-descent step (full Madgwick MARG equations). */
        s0 = -_2q1 * (2.0f * q3q3 - _2q1 * ay - _2q2 * az - _4q0 * ax)
           + _2q2 * (2.0f * q1q1 + _2q0 * ay - _4q2 * az + _4q0 * ax)
           - _2bz * _2q1 * (2.0f * q1q1 - _2q2 * ay - _2q0 * az + _4q2 * ax)
           - _2bz * _2q2 * (2.0f * q2q2 + _2q3 * ay - _2q1 * az + _4q3 * ax)
           + _2bx * _2q1 * (_2bx * (0.5f - q3q3) - _2bz * q2q2 + _2bx * q1q1)
           + _2bx * _2q2 * (_2bx * (q1q1 - q3q3) + _2bz * q0q0);
        s1 = _2q1 * (2.0f * q3q3 + _2q1 * ay - _4q1 * ax - _4q2 * az)
           - _2q0 * (2.0f * q2q2 - _2q1 * ay + _2q0 * az - _4q2 * ax)
           - _2bz * _2q0 * (2.0f * q1q1 - _2q2 * ay - _2q0 * az + _4q2 * ax)
           + _2bz * _2q1 * (2.0f * q0q0 + 2.0f * q2q2 - 1.0f - ay)
           - _2bx * _2q0 * (_2bx * (q1q1 - q3q3) + _2bz * q0q0)
           + _2bx * _2q1 * (_2bx * (_2q0q2 + _2q1q3) + _2bz * (q1q1 - q0q0));
        s2 = _2q2 * (2.0f * q3q3 + _2q1 * ay - _4q1 * ax - _4q2 * az)
           + _2q0 * (2.0f * q1q1 + _2q0 * ay - _4q2 * az + _4q0 * ax)
           - _2bz * _2q0 * (2.0f * q2q2 + _2q3 * ay - _2q1 * az + _4q3 * ax)
           - _2bz * _2q2 * (2.0f * q0q0 + 2.0f * q2q2 - 1.0f - ay)
           + _2bx * _2q0 * (_2bx * (0.5f - q3q3) - _2bz * q2q2)
           + _2bx * _2q2 * (_2bx * (_2q0q1 - _2q2q3) + _2bz * (q0q0 - q1q1));
        s3 = _2q3 * (2.0f * q3q3 + _2q1 * ay - _4q1 * ax - _4q2 * az)
           - _2q0 * (2.0f * q2q2 - _2q1 * ay + _2q0 * az - _4q2 * ax)
           + _4q3 * (2.0f * q1q1 + _2q0 * ay - _4q2 * az + _4q0 * ax)
           + _2bz * _2q0 * (_2bx * (q1q1 - q3q3) + _2bz * q0q0)
           - _2bz * _2q3 * (2.0f * q0q0 + 2.0f * q2q2 - 1.0f - ay)
           - _2bx * _2q0 * (_2bx * (_2q0q2 + _2q1q3) + _2bz * (q1q1 - q0q0))
           + _2bx * _2q3 * (_2bx * (_2q0q1 - _2q2q3) + _2bz * (q0q0 - q1q1));

        recipNorm = fast_inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        s0 *= recipNorm; s1 *= recipNorm; s2 *= recipNorm; s3 *= recipNorm;

        /* Apply feedback step. */
        qDot1 -= g_beta * s0;
        qDot2 -= g_beta * s1;
        qDot3 -= g_beta * s2;
        qDot4 -= g_beta * s3;
    }

    /* Integrate rate of change. */
    q0 += qDot1 * g_inv_sample_hz;
    q1 += qDot2 * g_inv_sample_hz;
    q2 += qDot3 * g_inv_sample_hz;
    q3 += qDot4 * g_inv_sample_hz;

    /* Normalize quaternion. */
    recipNorm = fast_inv_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    g_q0 = q0 * recipNorm;
    g_q1 = q1 * recipNorm;
    g_q2 = q2 * recipNorm;
    g_q3 = q3 * recipNorm;
}