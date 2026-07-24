/*
 * classifier.h — k-NN alloy classifier with lift-off compensation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_CLASSIFIER_H
#define ALLOYSCAN_CLASSIFIER_H

#include <stdint.h>
#include <stdbool.h>
#include "alloy_db.h"

/* Classification result structure */
typedef struct {
    int         alloy_index;        /* Index into alloy_database, -1 if none */
    float       confidence;         /* 0.0 to 1.0 */
    float       distance;           /* Euclidean distance to nearest match */
    int         alternatives[MAX_ALTERNATIVES];  /* Next-best indices */
    float       alt_distances[MAX_ALTERNATIVES];  /* Their distances  */
    int         alt_count;          /* Number of valid alternatives */
    bool        edge_warning;       /* True if edge effect detected   */
    bool        uncertain;          /* True if confidence < threshold */
    float       feature[8];         /* The compensated feature vector */
    float       liftoff_mm;         /* Measured lift-off distance     */
} classify_result_t;

/* Calibration data stored in flash. The lift-off direction vectors define
 * the axis along which lift-off variation projects, so we can project it
 * out of the measured feature. */
typedef struct {
    float liftoff_dir[8];          /* Unit vector for lift-off axis     */
    float baseline[8];             /* Open-circuit reference (zero)     */
    float ref_block[8];            /* Reference 6061 block at contact  */
    float scale[8];                 /* Per-dimension normalization scale */
    bool  valid;                   /* Calibration data is valid         */
    uint32_t magic;                /* 0xA10YCA57 = "ALOYCAS"          */
} calibration_t;

#define CALIBRATION_MAGIC 0xA10C4A57UL

/* Global calibration data (loaded from flash at startup) */
extern calibration_t g_calibration;

/* Initialize classifier — loads calibration from flash */
void classifier_init(void);

/* Classify a raw I/Q measurement into an alloy
 * raw_feature[8]: raw I/Q at 4 frequencies (from lock-in)
 * liftoff_mm: measured probe-to-surface distance
 * Returns classification result
 */
classify_result_t classifier_classify(const float raw_feature[8], float liftoff_mm);

/* Perform factory calibration step
 * step 0: open circuit (air)
 * step 1: reference block contact
 * step 2: reference block + 0.5mm shim
 * step 3: reference block + 1.0mm shim
 * Returns true on success
 */
bool classifier_calibrate(int step, const float raw_feature[8], float liftoff_mm);

/* Save calibration to flash */
bool classifier_save_calibration(void);

/* Load calibration from flash */
bool classifier_load_calibration(void);

/* Check if calibration is valid */
bool classifier_is_calibrated(void);

/* Train a custom alloy: add a new entry from averaged measurements
 * name: alloy name (up to 15 chars)
 * measurements: array of raw_feature[8] arrays
 * count: number of measurements to average
 * Returns true on success
 */
bool classifier_train_custom(const char *name, const float measurements[][8],
                             int count);

/* Compute Euclidean distance between two 8-dim vectors */
float classifier_distance(const float a[8], const float b[8]);

#endif /* ALLOYSCAN_CLASSIFIER_H */