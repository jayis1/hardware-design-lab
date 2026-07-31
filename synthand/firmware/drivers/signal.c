/*
 * signal.c — Signal processing for Synthand.
 *
 * Implements bandpass filtering, RMS envelope extraction, gravity separation,
 * complementary filter quaternion estimation, and feature vector assembly.
 * All operations use fixed-point arithmetic (Q15/Q29) for deterministic timing.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include <stdint.h>
#include <string.h>
#include "board.h"
#include "registers.h"
#include "drivers/signal.h"

/* -------------------------------------------------------------------------
 * Fixed-point constants
 * Author: jayis1
 * ------------------------------------------------------------------------- */
#define Q15_ONE          0x7FFF
#define Q15_HALF         0x4000
#define Q15_PI           0x6488    /* π in Q15 ≈ 3.14159 */

/* Gravity constant in accelerometer LSB (±16g, 16-bit: 1g = 2048 LSB) */
#define GRAVITY_LSB      2048

/* EMG bandpass filter coefficients (2nd-order Butterworth, 20-450 Hz at 500 Hz Fs)
 * Pre-computed Q15 coefficients for the biquad cascade.
 * High-pass stage (fc=20Hz): b0, b1, b2, a1, a2
 * Low-pass stage (fc=450Hz): b0, b1, b2, a1, a2
 * Author: jayis1 */
static const q15_t bp_hp_b0 = 0x7B50;  /* ≈ 0.9635 */
static const q15_t bp_hp_b1 = 0x84AF;  /* ≈ -0.9635 (neg) */
static const q15_t bp_hp_b2 = 0x7B50;
static const q15_t bp_hp_a1 = 0x84AF;  /* ≈ -0.9635 */
static const q15_t bp_hp_a2 = 0x76A0;

static const q15_t bp_lp_b0 = 0x0F82;
static const q15_t bp_lp_b1 = 0x1F04;
static const q15_t bp_lp_b2 = 0x0F82;
static const q15_t bp_lp_a1 = 0x6E4B;
static const q15_t bp_lp_a2 = 0xCE51;

/* Filter state — per EMG channel */
typedef struct {
    q15_t x1, x2;   /* input history */
    q15_t y1, y2;   /* output history (high-pass stage) */
    q15_t z1, z2;   /* output history (low-pass stage) */
} emg_filter_state_t;

static emg_filter_state_t emg_filter[NUM_EMG_CHANNELS];

/* RMS envelope window (10 samples = 20 ms at 500 Hz) */
#define RMS_WINDOW_SIZE 10
static int32_t rms_window[NUM_EMG_CHANNELS][RMS_WINDOW_SIZE];
static int rms_idx[NUM_EMG_CHANNELS] = {0};
static int32_t rms_sum[NUM_EMG_CHANNELS] = {0};

/* Gravity estimation state (low-pass filter on accelerometer) */
static q15_t gravity_est[NUM_IMUS][3];
static int gravity_init_done = 0;

/* Wrist quaternion state (complementary filter) */
static q29_t wrist_q[4] = {0x20000000, 0, 0, 0};  /* Q29: (1.0, 0, 0, 0) */
static uint32_t last_quat_update_us = 0;

/* Vibrato detection state — per finger */
#define VIB_BUF_SIZE 40  /* 80 ms at 500 Hz */
static q15_t vib_buf[NUM_FINGERS][VIB_BUF_SIZE];
static int vib_idx[NUM_FINGERS] = {0};

/* -------------------------------------------------------------------------
 * Q15 multiply (saturating)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
static q15_t q15_mul(q15_t a, q15_t b)
{
    int32_t result = ((int32_t)a * (int32_t)b) >> 15;
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    return (q15_t)result;
}

/* -------------------------------------------------------------------------
 * Initialize signal processing state
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void signal_init(const calibration_t *calib)
{
    (void)calib;

    memset(emg_filter, 0, sizeof(emg_filter));
    memset(rms_window, 0, sizeof(rms_window));
    memset(rms_sum, 0, sizeof(rms_sum));
    memset(rms_idx, 0, sizeof(rms_idx));
    memset(gravity_est, 0, sizeof(gravity_est));
    memset(vib_buf, 0, sizeof(vib_buf));
    memset(vib_idx, 0, sizeof(vib_idx));

    gravity_init_done = 0;
    wrist_q[0] = 0x20000000;  /* Q29: 1.0 */
    wrist_q[1] = 0;
    wrist_q[2] = 0;
    wrist_q[3] = 0;
}

/* -------------------------------------------------------------------------
 * EMG bandpass filter (2nd-order Butterworth, 20-450 Hz)
 * Biquad cascade: high-pass (20 Hz) → low-pass (450 Hz)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
q15_t signal_emg_bandpass(q15_t sample, int channel)
{
    emg_filter_state_t *f = &emg_filter[channel];

    /* High-pass stage */
    q15_t hp_out = q15_mul(bp_hp_b0, sample) +
                   q15_mul(bp_hp_b1, f->x1) +
                   q15_mul(bp_hp_b2, f->x2) -
                   q15_mul(bp_hp_a1, f->y1) -
                   q15_mul(bp_hp_a2, f->y2);

    /* Update high-pass state */
    f->x2 = f->x1;
    f->x1 = sample;
    f->y2 = f->y1;
    f->y1 = hp_out;

    /* Low-pass stage */
    q15_t lp_out = q15_mul(bp_lp_b0, hp_out) +
                   q15_mul(bp_lp_b1, f->y1) +
                   q15_mul(bp_lp_b2, f->y2) -
                   q15_mul(bp_lp_a1, f->z1) -
                   q15_mul(bp_lp_a2, f->z2);

    f->z2 = f->z1;
    f->z1 = lp_out;

    return lp_out;
}

/* -------------------------------------------------------------------------
 * RMS envelope extraction (20 ms window = 10 samples at 500 Hz)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
q15_t signal_emg_envelope(int32_t raw_sample, int channel)
{
    /* Convert 24-bit raw to Q15 (shift right 9 bits) */
    q15_t q15_sample = (q15_t)(raw_sample >> 9);

    /* Bandpass filter */
    q15_t filtered = signal_emg_bandpass(q15_sample, channel);

    /* Rectify (absolute value) */
    int32_t rect = (filtered < 0) ? -filtered : filtered;

    /* Sliding window RMS: subtract old, add new */
    rms_sum[channel] -= rms_window[channel][rms_idx[channel]];
    rms_window[channel][rms_idx[channel]] = rect * rect;
    rms_sum[channel] += rms_window[channel][rms_idx[channel]];
    rms_idx[channel] = (rms_idx[channel] + 1) % RMS_WINDOW_SIZE;

    /* Compute RMS = sqrt(sum / N) — use integer sqrt approximation */
    int32_t mean_sq = rms_sum[channel] / RMS_WINDOW_SIZE;
    /* Quick integer sqrt (Newton's method, 3 iterations) */
    int32_t s = mean_sq;
    if (s > 0) {
        int32_t x = s;
        for (int i = 0; i < 3; i++) {
            x = (x + s / x) / 2;
            if (x == 0) x = 1;
        }
        s = x;
    }

    /* Normalize to Q15 (0 to 0x7FFF) */
    if (s > Q15_ONE) s = Q15_ONE;
    return (q15_t)s;
}

/* -------------------------------------------------------------------------
 * Gravity separation — low-pass filter on accelerometer to extract gravity
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void signal_gravity_separate(const int16_t accel_in[3],
                              q15_t accel_linear_out[3],
                              int imu_index)
{
    /* Convert raw to Q15 (±16g → ±32768, so divide by 16) */
    q15_t ax = (q15_t)(accel_in[0] / 16);
    q15_t ay = (q15_t)(accel_in[1] / 16);
    q15_t az = (q15_t)(accel_in[2] / 16);

    if (!gravity_init_done) {
        gravity_est[imu_index][0] = ax;
        gravity_est[imu_index][1] = ay;
        gravity_est[imu_index][2] = az;
    } else {
        /* Low-pass: gravity = 0.98 * gravity + 0.02 * accel */
        q15_t alpha = 0x7EB8;  /* ≈ 0.98 in Q15 */
        q15_t one_minus_alpha = 0x0470;  /* ≈ 0.02 */
        gravity_est[imu_index][0] = q15_mul(alpha, gravity_est[imu_index][0]) +
                                     q15_mul(one_minus_alpha, ax);
        gravity_est[imu_index][1] = q15_mul(alpha, gravity_est[imu_index][1]) +
                                     q15_mul(one_minus_alpha, ay);
        gravity_est[imu_index][2] = q15_mul(alpha, gravity_est[imu_index][2]) +
                                     q15_mul(one_minus_alpha, az);
    }

    /* Linear acceleration = total - gravity */
    accel_linear_out[0] = ax - gravity_est[imu_index][0];
    accel_linear_out[1] = ay - gravity_est[imu_index][1];
    accel_linear_out[2] = az - gravity_est[imu_index][2];
}

/* -------------------------------------------------------------------------
 * Complementary filter for wrist quaternion
 * Fuses gyro integration (high-frequency) with accel gravity vector (low-freq)
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void signal_quaternion_update(q29_t quat[4],
                               const int16_t gyro[3],
                               const int16_t accel[3],
                               uint32_t dt_us)
{
    /* Convert gyro to radians/sec in Q29.
     * ±2000 dps range, 16-bit: 1 LSB = 2000/32768 dps = 0.061 dps
     * Convert to rad/s: × π/180 = × 0.017453
     * In Q29: × 0.017453 × 2^29 ≈ × 9356443 */
    int64_t gx = (int64_t)gyro[0] * 9356443LL / (1LL << 29);
    int64_t gy = (int64_t)gyro[1] * 9356443LL / (1LL << 29);
    int64_t gz = (int64_t)gyro[2] * 9356443LL / (1LL << 29);

    /* dt in seconds (Q29) */
    int64_t dt = (int64_t)dt_us * 512LL;  /* µs to Q29 seconds (×2^29/1e6 ≈ ×512) */

    /* Quaternion derivative: q_dot = 0.5 * q ⊗ [0, gx, gy, gz] * dt */
    q29_t w = quat[0], x = quat[1], y = quat[2], z = quat[3];

    /* Integrate gyro: q_new = q + 0.5 * q ⊗ omega * dt */
    int64_t dw = (int64_t)(-x * gx - y * gy - z * gz) * dt >> 29;
    int64_t dx = (int64_t)(w * gx + y * gz - z * gy) * dt >> 29;
    int64_t dy = (int64_t)(w * gy - x * gz + z * gx) * dt >> 29;
    int64_t dz = (int64_t)(w * gz + x * gy - y * gx) * dt >> 29;

    quat[0] = (q29_t)(w + dw / 2);
    quat[1] = (q29_t)(x + dx / 2);
    quat[2] = (q29_t)(y + dy / 2);
    quat[3] = (q29_t)(z + dz / 2);

    /* Normalize quaternion (avoid drift) */
    int64_t norm_sq = (int64_t)quat[0]*quat[0] + (int64_t)quat[1]*quat[1] +
                      (int64_t)quat[2]*quat[2] + (int64_t)quat[3]*quat[3];
    int64_t target = (int64_t)0x20000000 * 0x20000000;  /* 1.0 in Q29² */
    if (norm_sq > 0) {
        /* Simple normalization: divide by sqrt(norm) — approximate */
        int64_t norm = norm_sq;
        for (int i = 0; i < 4; i++) {
            int64_t inv = target / norm;
            norm = (norm + inv) / 2;
        }
        int64_t scale = target / norm;
        quat[0] = (q29_t)((int64_t)quat[0] * scale >> 29);
        quat[1] = (q29_t)((int64_t)quat[1] * scale >> 29);
        quat[2] = (q29_t)((int64_t)quat[2] * scale >> 29);
        quat[3] = (q29_t)((int64_t)quat[3] * scale >> 29);
    }

    /* TODO: Accelerometer correction (gravity vector → quaternion correction)
     * This would use a complementary filter to correct gyro drift using
     * the accel gravity direction. Omitted for brevity in this demo. */
    (void)accel;
}

/* -------------------------------------------------------------------------
 * Estimate finger curl from gravity vectors
 * Compares finger gravity direction to wrist gravity direction.
 * Returns Q15: 0 = straight, 0x7FFF = fully curled.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
q15_t signal_estimate_curl(const q15_t finger_grav[3],
                            const q15_t wrist_grav[3])
{
    /* Dot product of normalized gravity vectors.
     * cos(θ) = (fg · wg) / (|fg| * |wg|)
     * curl = 1 - cos(θ) → 0 when aligned (straight), 1 when perpendicular (90° curl) */
    int32_t dot = (int32_t)q15_mul(finger_grav[0], wrist_grav[0]) +
                  (int32_t)q15_mul(finger_grav[1], wrist_grav[1]) +
                  (int32_t)q15_mul(finger_grav[2], wrist_grav[2]);

    /* dot is in Q15 (sum of 3 Q15 muls, each >> 15) */
    dot = dot >> 1;  /* approximate normalization */

    /* curl = Q15_ONE - dot (clamp to 0..Q15_ONE) */
    int32_t curl = Q15_ONE - dot;
    if (curl < 0) curl = 0;
    if (curl > Q15_ONE) curl = Q15_ONE;
    return (q15_t)curl;
}

/* -------------------------------------------------------------------------
 * Estimate tap/strike velocity from linear acceleration magnitude
 * Author: jayis1
 * ------------------------------------------------------------------------- */
q15_t signal_estimate_velocity(const q15_t accel_linear[3])
{
    /* Magnitude = sqrt(ax² + ay² + az²) */
    int32_t mag_sq = (int32_t)accel_linear[0] * accel_linear[0] +
                     (int32_t)accel_linear[1] * accel_linear[1] +
                     (int32_t)accel_linear[2] * accel_linear[2];
    /* Integer sqrt */
    int32_t mag = mag_sq;
    if (mag > 0) {
        int32_t x = mag;
        for (int i = 0; i < 4; i++) {
            x = (x + mag / x) / 2;
            if (x == 0) x = 1;
        }
        mag = x;
    }
    /* Scale to 0-127 MIDI velocity range */
    if (mag > Q15_ONE) mag = Q15_ONE;
    return (q15_t)mag;
}

/* -------------------------------------------------------------------------
 * Vibrato detection — 4-8 Hz oscillation in curl signal
 * Checks for zero-crossings in the curl buffer.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
q15_t signal_detect_vibrato(q15_t curl_value, int finger)
{
    /* Store in circular buffer */
    vib_buf[finger][vib_idx[finger]] = curl_value;
    vib_idx[finger] = (vib_idx[finger] + 1) % VIB_BUF_SIZE;

    /* Count zero-crossings around the mean in the buffer */
    /* First compute mean */
    int32_t sum = 0;
    for (int i = 0; i < VIB_BUF_SIZE; i++) {
        sum += vib_buf[finger][i];
    }
    q15_t mean = (q15_t)(sum / VIB_BUF_SIZE);

    /* Count zero-crossings of (sample - mean) */
    int crossings = 0;
    int prev_sign = 0;
    for (int i = 0; i < VIB_BUF_SIZE; i++) {
        int sign = (vib_buf[finger][i] > mean) ? 1 : -1;
        if (prev_sign != 0 && sign != prev_sign) {
            crossings++;
        }
        prev_sign = sign;
    }

    /* 4-8 Hz at 500 Hz over 80 ms window: expect 0.6-1.3 cycles = 1-3 crossings */
    if (crossings >= 1 && crossings <= 5) {
        /* Compute amplitude (peak-to-peak / 2) */
        q15_t min_val = Q15_ONE, max_val = 0;
        for (int i = 0; i < VIB_BUF_SIZE; i++) {
            if (vib_buf[finger][i] < min_val) min_val = vib_buf[finger][i];
            if (vib_buf[finger][i] > max_val) max_val = vib_buf[finger][i];
        }
        q15_t amplitude = (max_val - min_val) / 2;
        return amplitude;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * Main feature extraction — called at 500 Hz
 * Assembles the 38-feature vector from IMU and EMG data.
 * Author: jayis1
 * ------------------------------------------------------------------------- */
void signal_extract_features(const imu_sample_t *imu,
                              const emg_sample_t *emg,
                              const calibration_t *calib,
                              feature_vector_t *features)
{
    /* Process EMG: 5 channels → bandpass → RMS envelope */
    for (int ch = 0; ch < NUM_EMG_CHANNELS; ch++) {
        int32_t raw = emg->channel[ch];
        /* Apply calibration: subtract baseline, normalize by MVC */
        raw -= (int32_t)calib->emg_baseline[ch] << 9;
        q15_t env = signal_emg_envelope(raw, ch);
        /* Normalize: env / (MVC - baseline) in Q15 */
        int32_t range = (int32_t)calib->emg_mvc[ch] - calib->emg_baseline[ch];
        if (range > 0) {
            env = (q15_t)((int32_t)env * Q15_ONE / range);
        }
        features->emg_envelope[ch] = env;
    }

    /* Process IMU: 6 chips (5 fingers + wrist) */
    q15_t wrist_grav[3] = {0};

    for (int i = 0; i < NUM_IMUS; i++) {
        q15_t linear_accel[3];
        signal_gravity_separate(imu[i].accel, linear_accel, i);

        if (i < NUM_FINGERS) {
            /* Finger IMU data → features */
            for (int axis = 0; axis < 3; axis++) {
                /* Convert raw 16-bit to Q15 (±16g → ±32768, /16 for Q15) */
                features->finger_accel[i][axis] = (q15_t)(imu[i].accel[axis] / 16);
                features->finger_gyro[i][axis] = (q15_t)(imu[i].gyro[axis] / 16);
            }

            /* Estimate finger curl from gravity direction */
            q15_t finger_grav[3] = {
                gravity_est[i][0],
                gravity_est[i][1],
                gravity_est[i][2]
            };
            features->finger_curl[i] = signal_estimate_curl(finger_grav, wrist_grav);

            /* Estimate velocity from linear acceleration */
            features->finger_velocity[i] = signal_estimate_velocity(linear_accel);

            /* Vibrato detection */
            (void)signal_detect_vibrato(features->finger_curl[i], i);

        } else {
            /* Wrist IMU (i == 5) */
            for (int axis = 0; axis < 3; axis++) {
                features->wrist_accel[axis] = (q15_t)(imu[i].accel[axis] / 16);
                wrist_grav[axis] = gravity_est[i][axis];
            }

            /* Update wrist quaternion via complementary filter */
            uint32_t dt_us = 2000;  /* 2 ms = 2000 µs */
            signal_quaternion_update(wrist_q, imu[i].gyro, imu[i].accel, dt_us);
            features->wrist_quat[0] = wrist_q[0];
            features->wrist_quat[1] = wrist_q[1];
            features->wrist_quat[2] = wrist_q[2];
            features->wrist_quat[3] = wrist_q[3];
        }
    }

    gravity_init_done = 1;
    features->timestamp = 0;  /* filled by caller */
}

/*
 * Author: jayis1
 * End of signal.c
 */