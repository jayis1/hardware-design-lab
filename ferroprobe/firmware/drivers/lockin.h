/*
 * lockin.h — Digital lock-in detection engine interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_LOCKIN_H
#define FERROPROBE_LOCKIN_H

#include <stdint.h>

/* Measurement output rates (configurable averaging) */
typedef enum {
    LOCKIN_RATE_FAST = 0,      /* 100 Hz, 4x averaging     */
    LOCKIN_RATE_SURVEY = 1,     /* 10 Hz, 40x averaging      */
    LOCKIN_RATE_PRECISION = 2,  /* 1 Hz, 400x averaging       */
    LOCKIN_RATE_ULTRA = 3,      /* 0.1 Hz, 4000x averaging   */
} lockin_rate_t;

/* 3-axis field measurement result in nanotesla */
typedef struct {
    float bx_nt;     /* X-axis field, nT (calibrated) */
    float by_nt;     /* Y-axis field, nT (calibrated) */
    float bz_nt;     /* Z-axis field, nT (calibrated) */
    float b_total;   /* |B| total field magnitude, nT */
    float bx_raw;    /* Raw (uncalibrated) X, nT */
    float by_raw;    /* Raw Y, nT */
    float bz_raw;    /* Raw Z, nT */
    uint32_t timestamp_ms;
    uint8_t  valid;
} field_measurement_t;

/* Initialize the lock-in engine.
 * Configures TIM2 for ADC sampling trigger, allocates buffers.
 */
void lockin_init(void);

/* Feed a raw ADC sample from one axis into the lock-in pipeline.
 * Called from the ADC interrupt at 30 kSPS per channel.
 * axis: 0=X, 1=Y, 2=Z, 3=Temperature
 * raw: 24-bit signed ADC value
 */
void lockin_feed_sample(uint8_t axis, int32_t raw);

/* Process accumulated lock-in data and produce a field measurement.
 * Called from the main loop at a rate >= output rate.
 * Returns 1 if a new measurement is available, 0 otherwise.
 */
uint8_t lockin_process(field_measurement_t *result);

/* Set the output data rate */
void lockin_set_rate(lockin_rate_t rate);

/* Get the current output data rate */
lockin_rate_t lockin_get_rate(void);

/* Set the phase offset of the 2f0 reference (in degrees).
 * Used to compensate for sensor and preamplifier phase shift.
 */
void lockin_set_phase(float phase_deg);

/* Get the current I and Q components for a given axis (debug/calib) */
void lockin_get_iq(uint8_t axis, float *i_out, float *q_out);

#endif /* FERROPROBE_LOCKIN_H */