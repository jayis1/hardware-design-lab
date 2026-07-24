/*
 * calibration.h — Factory calibration routines
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_CALIBRATION_H
#define ALLOYSCAN_CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>

/* Calibration step descriptions */
extern const char *calibration_step_names[4];

/* Run the full 4-step calibration sequence
 * Each step requires the user to position the probe and press OK.
 * Returns true if calibration completed successfully. */
bool calibration_run_full(void);

/* Run a single calibration step
 * step 0: Open circuit (air)
 * step 1: Reference block (contact)
 * step 2: Reference block + 0.5mm shim
 * step 3: Reference block + 1.0mm shim
 * Returns true on success */
bool calibration_run_step(int step);

/* Check calibration status
 * Returns: 0 = not calibrated, 1 = calibrated, -1 = drift detected */
int calibration_status(void);

/* Reset calibration (forces recalibration) */
void calibration_reset(void);

#endif /* ALLOYSCAN_CALIBRATION_H */