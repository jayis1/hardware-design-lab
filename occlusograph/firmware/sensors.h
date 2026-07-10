/*
 * sensors.h — Sensor acquisition interface for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_SENSORS_H
#define OCCLUSOGRAPH_SENSORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/* Raw frame: one sample of every element at a given instant.
 * piezo[64]  — 12-bit ADC counts from charge amps (signed after zeroing).
 * cap[32]    — 24-bit CDC codes (converted to signed 32-bit).
 * imu[6]     — accel xyz, gyro xyz (16-bit, raw).
 * temp_mc    — intraoral temperature in milli-Celsius.
 * contact    — true if MAX30101 reflectance confirms tissue contact.
 * timestamp  — frame counter (increments at SAMPLE_RATE_HZ).
 */
typedef struct {
    int16_t  piezo[PIEZO_NUM_ELEMENTS];
    int32_t  cap[CAP_NUM_ELEMENTS];
    int16_t  imu[6];
    int32_t  temp_mc;
    bool     contact;
    uint32_t timestamp;
} sensor_frame_t;

/* Calibrated force frame (output of fusion, input to classifier).
 * force_mN[64] — per-element fused force in millinewtons (signed).
 * A negative value means tension/lift (rare for bite, used for diagnostics).
 */
typedef struct {
    int16_t force_mN[PIEZO_NUM_ELEMENTS];
    int16_t imu_ax, imu_ay, imu_az;   /* mg */
    int16_t imu_gx, imu_gy, imu_gz;   /* mdps */
    uint32_t timestamp;
} force_frame_t;

/* Public API */
int  sensors_init(void);
void sensors_start(void);
void sensors_stop(void);

/* Blocking acquire of one full frame. Called from the sensor thread at 1 kHz.
 * Returns 0 on success, negative on error (then the caller skips the frame).
 */
int  sensors_acquire(sensor_frame_t *frame);

/* Apply per-element calibration offsets (from flash). */
void sensors_apply_calibration(const int16_t piezo_off[PIEZO_NUM_ELEMENTS],
                               const int32_t cap_off[CAP_NUM_ELEMENTS]);

/* Trigger a charge-amp reset (manual). */
void sensors_piezo_reset(void);

/* Read battery voltage in millivolts. */
uint16_t sensors_read_battery_mv(void);

/* Read temperature in milli-Celsius. */
int32_t sensors_read_temp_mc(void);

/* True if MAX30101 proximity detects the device is seated against tissue. */
bool sensors_check_contact(void);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_SENSORS_H */