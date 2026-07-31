/*
 * signal.h — Signal processing for Synthand.
 *
 * Bandpass filter, RMS envelope extraction, gravity separation,
 * quaternion estimation, and feature vector assembly.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_SIGNAL_H
#define SYNTHAND_SIGNAL_H

#include <stdint.h>
#include "board.h"
#include "drivers/imu.h"
#include "drivers/emg.h"

/* Q15 fixed-point type (16-bit signed, range -1.0 to +0.99997) */
typedef int16_t q15_t;

/* Q29 fixed-point for quaternion components (range -1.0 to +1.0) */
typedef int32_t q29_t;

/* Processed feature vector — output of signal_extract_features()
 * This is the input to the TCN gesture classifier. */
typedef struct {
    /* EMG envelopes (5 channels, Q15: 0 to 0x7FFF = 0.0 to 1.0) */
    q15_t emg_envelope[NUM_EMG_CHANNELS];

    /* Per-finger accel (5 fingers × 3 axes, Q15, ±16g mapped) */
    q15_t finger_accel[NUM_FINGERS][3];

    /* Per-finger gyro (5 fingers × 3 axes, Q15, ±2000 dps mapped) */
    q15_t finger_gyro[NUM_FINGERS][3];

    /* Wrist accel (3 axes, Q15) */
    q15_t wrist_accel[3];

    /* Derived features */
    q15_t finger_curl[NUM_FINGERS];     /* 0 = straight, 0x7FFF = fully curled */
    q15_t finger_velocity[NUM_FINGERS]; /* tap/strike velocity estimate */

    /* Wrist orientation quaternion (Q29) */
    q29_t wrist_quat[4];                /* w, x, y, z */

    /* Timestamp */
    uint32_t timestamp;
} feature_vector_t;

/* Initialize signal processing state (filters, bias registers) */
void signal_init(const calibration_t *calib);

/* Process raw sensor data into a feature vector.
 * Called at 500 Hz (every 2 ms). */
void signal_extract_features(const imu_sample_t *imu,
                             const emg_sample_t *emg,
                             const calibration_t *calib,
                             feature_vector_t *features);

/* Bandpass filter for EMG (20-450 Hz, 2nd-order Butterworth, Q15).
 * Processes one sample through the IIR cascade. */
q15_t signal_emg_bandpass(q15_t sample, int channel);

/* RMS envelope extraction with 20 ms window (10 samples at 500 Hz).
 * Returns Q15 envelope value. */
q15_t signal_emg_envelope(int32_t raw_sample, int channel);

/* Gravity separation from accelerometer data.
 * Removes the gravity component from accel, leaving linear acceleration. */
void signal_gravity_separate(const int16_t accel_in[3],
                             q15_t accel_linear_out[3],
                             int imu_index);

/* Complementary filter for wrist quaternion estimation.
 * Fuses gyro rate with accel gravity vector. */
void signal_quaternion_update(q29_t quat[4],
                              const int16_t gyro[3],
                              const int16_t accel[3],
                              uint32_t dt_us);

/* Estimate finger curl angle from gravity vector rotation.
 * Compares finger accel gravity direction to wrist accel gravity direction. */
q15_t signal_estimate_curl(const q15_t finger_grav[3],
                           const q15_t wrist_grav[3]);

/* Estimate tap/strike velocity from peak acceleration. */
q15_t signal_estimate_velocity(const q15_t accel_linear[3]);

/* Vibrato detection — checks for 4-8 Hz oscillation in curl signal.
 * Returns amplitude (Q15) if vibrato detected, 0 otherwise. */
q15_t signal_detect_vibrato(q15_t curl_value, int finger);

#endif /* SYNTHAND_SIGNAL_H */