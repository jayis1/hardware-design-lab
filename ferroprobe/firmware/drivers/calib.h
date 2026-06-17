/*
 * calib.h — Calibration and orientation compensation interface
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_CALIB_H
#define FERROPROBE_CALIB_H

#include <stdint.h>
#include "lockin.h"
#include "imu.h"

/* Calibration parameters */
typedef struct {
    float offset[3];        /* Per-axis offset (nT) */
    float scale[3];          /* Per-axis scale factor */
    float cross[3][3];       /* Cross-axis correction matrix */
    float temp_coeff[3];    /* Temperature coefficient (nT/°C) */
    float phase_deg[3];      /* Lock-in phase offset per axis (degrees) */
    uint8_t valid;           /* 1 if calibration has been computed */
} calib_params_t;

/* Initialize the calibration module */
void calib_init(void);

/* Start collecting samples for ellipsoid calibration.
 * The user rotates the device through all orientations (figure-8 motion).
 * Samples are collected for a configurable duration (default 30 s).
 */
void calib_start_collection(uint16_t duration_s);

/* Feed a raw field measurement into the calibration collector.
 * Called from the main loop at the lock-in output rate.
 */
void calib_collect_sample(const field_measurement_t *field, const imu_data_t *imu);

/* Check if calibration collection is complete */
uint8_t calib_collection_complete(void);

/* Compute the calibration parameters from collected samples.
 * Performs a least-squares ellipsoid fit to find offset, scale, and
 * cross-axis corrections.  Stores the result and applies it to the
 * lock-in engine.
 */
void calib_compute(void);

/* Get the current calibration parameters */
const calib_params_t *calib_get(void);

/* Apply orientation compensation: rotate the measured magnetic vector
 * from the sensor frame to the Earth-level frame using IMU orientation.
 * Modifies the field_measurement_t in place.
 */
void calib_apply_orientation(field_measurement_t *field, const imu_data_t *imu);

/* Set custom calibration parameters (from BLE or stored in flash) */
void calib_set(const calib_params_t *params);

/* Save calibration to flash memory */
uint8_t calib_save_to_flash(void);

/* Load calibration from flash memory */
uint8_t calib_load_from_flash(void);

#endif /* FERROPROBE_CALIB_H */