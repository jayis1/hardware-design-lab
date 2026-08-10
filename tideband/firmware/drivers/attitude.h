/**
 * @file    attitude.h
 * @brief   TideBand — Attitude estimation (IMU + magnetometer) API.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#ifndef TIDEBAND_ATTITUDE_H
#define TIDEBAND_ATTITUDE_H

#include <stdint.h>

/* ---- Attitude representation ---- */
typedef struct {
    float roll;     /* Rotation about X (phi), radians */
    float pitch;    /* Rotation about Y (theta), radians */
    float yaw;      /* Rotation about Z (psi), radians */
    float dcm[3][3]; /* Direction cosine matrix (body → NED) */
    float gyro[3];  /* Latest gyro rates (rad/s) */
    float accel[3]; /* Latest accel (m/s²) */
    float mag[3];   /* Latest magnetometer (gauss) */
    uint8_t valid;  /* 1 if attitude solution is valid */
} attitude_t;

/* ---- Public API ---- */

/** Initialize attitude subsystem (IMU SPI, mag I2C, bias calibration). */
void attitude_init(void);

/** Read raw sensor data and update attitude solution.
 *  Should be called at 100 Hz for best performance. */
void attitude_update(attitude_t *att);

/** Set gyro bias from stationary samples (call when device is still). */
void attitude_calibrate_gyro(void);

/** Set magnetometer hard/soft iron compensation.
 *  Requires the user to rotate the device through all orientations. */
void attitude_calibrate_mag(void);

/** Rotate a body-frame vector into the NED (Earth) frame using DCM. */
void attitude_rotate_to_ned(const attitude_t *att,
                             const float body[3], float ned[3]);

#endif /* TIDEBAND_ATTITUDE_H */