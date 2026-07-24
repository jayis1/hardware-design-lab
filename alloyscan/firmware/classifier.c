/*
 * classifier.c — k-NN alloy classifier with lift-off compensation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Implements the core classification algorithm:
 * 1. Subtract open-circuit baseline
 * 2. Apply per-dimension scale (from calibration)
 * 3. Project out lift-off component (dot product with lift-off direction)
 * 4. Normalize the resulting feature vector
 * 5. Compute Euclidean distance to all 64 reference alloys
 * 6. Select k=5 nearest, weighted voting for confidence
 * 7. Edge detection via cross-frequency residual analysis
 */

#include "classifier.h"
#include "alloy_db.h"
#include "board.h"
#include <string.h>
#include <math.h>

/* Global calibration data */
calibration_t g_calibration;

/* ---- Local helpers ---- */

static float vec_dot(const float a[8], const float b[8])
{
    float sum = 0.0f;
    for (int i = 0; i < 8; i++)
        sum += a[i] * b[i];
    return sum;
}

static void vec_sub(float out[8], const float a[8], const float b[8])
{
    for (int i = 0; i < 8; i++)
        out[i] = a[i] - b[i];
}

static void vec_scale(float v[8], float s)
{
    for (int i = 0; i < 8; i++)
        v[i] *= s;
}

static float vec_norm(const float v[8])
{
    return sqrtf(vec_dot(v, v));
}

static void vec_normalize(float v[8])
{
    float n = vec_norm(v);
    if (n > 1e-9f)
        vec_scale(v, 1.0f / n);
}

/* ---- Public API ---- */

float classifier_distance(const float a[8], const float b[8])
{
    float d = 0.0f;
    for (int i = 0; i < 8; i++) {
        float diff = a[i] - b[i];
        d += diff * diff;
    }
    return sqrtf(d);
}

void classifier_init(void)
{
    memset(&g_calibration, 0, sizeof(g_calibration));
    classifier_load_calibration();
}

bool classifier_is_calibrated(void)
{
    return g_calibration.valid && g_calibration.magic == CALIBRATION_MAGIC;
}

bool classifier_load_calibration(void)
{
    /* In a full implementation, this reads from external SPI flash
     * (W25Q128) at FLASH_CAL_ADDR. Here we simulate a valid calibration
     * with physically reasonable default values so the classifier
     * is functional out-of-box for testing. */
    if (!g_calibration.valid) {
        /* Default baseline (open circuit = zero signal) */
        for (int i = 0; i < 8; i++) {
            g_calibration.baseline[i] = 0.0f;
            g_calibration.scale[i] = 1.0f;
        }
        /* Default lift-off direction: primarily along the I (real) axis
         * at each frequency, since lift-off reduces signal magnitude. */
        g_calibration.liftoff_dir[0] =  0.8f;
        g_calibration.liftoff_dir[1] = -0.6f;
        g_calibration.liftoff_dir[2] =  0.8f;
        g_calibration.liftoff_dir[3] = -0.6f;
        g_calibration.liftoff_dir[4] =  0.8f;
        g_calibration.liftoff_dir[5] = -0.6f;
        g_calibration.liftoff_dir[6] =  0.8f;
        g_calibration.liftoff_dir[7] = -0.6f;
        vec_normalize(g_calibration.liftoff_dir);

        /* Reference block (6061 at contact) */
        g_calibration.ref_block[0] = 0.46f;
        g_calibration.ref_block[1] = 0.56f;
        g_calibration.ref_block[2] = 0.51f;
        g_calibration.ref_block[3] = 0.61f;
        g_calibration.ref_block[4] = 0.56f;
        g_calibration.ref_block[5] = 0.66f;
        g_calibration.ref_block[6] = 0.59f;
        g_calibration.ref_block[7] = 0.69f;

        g_calibration.valid = true;
        g_calibration.magic = CALIBRATION_MAGIC;
    }
    return g_calibration.valid;
}

bool classifier_save_calibration(void)
{
    /* In production: erase FLASH_CAL_ADDR sector, then write
     * g_calibration via SPI flash driver. Here we mark valid. */
    g_calibration.valid = true;
    g_calibration.magic = CALIBRATION_MAGIC;
    return true;
}

bool classifier_calibrate(int step, const float raw_feature[8], float liftoff_mm)
{
    static float cal_features[4][8];
    static float cal_liftoffs[4];

    if (step < 0 || step > 3)
        return false;

    memcpy(cal_features[step], raw_feature, sizeof(float) * 8);
    cal_liftoffs[step] = liftoff_mm;

    if (step == 0) {
        /* Open circuit: this is the baseline */
        memcpy(g_calibration.baseline, raw_feature, sizeof(float) * 8);
    } else if (step == 1) {
        /* Reference block at contact */
        memcpy(g_calibration.ref_block, raw_feature, sizeof(float) * 8);
    }

    if (step == 3) {
        /* Final step: compute lift-off direction from the 3 contact/shim
         * measurements (steps 1, 2, 3) by finding the principal direction
         * of variation. We approximate via PCA on the difference vectors. */

        float d1[8], d2[8], avg_dir[8];

        /* d1 = feature at 0.5mm - feature at contact */
        vec_sub(d1, cal_features[2], cal_features[1]);
        /* d2 = feature at 1.0mm - feature at 0.5mm */
        vec_sub(d2, cal_features[3], cal_features[2]);

        /* Average direction (approximation of first principal component) */
        for (int i = 0; i < 8; i++)
            avg_dir[i] = d1[i] + d2[i];

        vec_normalize(avg_dir);
        memcpy(g_calibration.liftoff_dir, avg_dir, sizeof(float) * 8);

        /* Compute scale so that reference block feature has unit norm */
        float ref_minus_baseline[8];
        vec_sub(ref_minus_baseline, g_calibration.ref_block, g_calibration.baseline);
        float rn = vec_norm(ref_minus_baseline);
        if (rn > 1e-6f) {
            for (int i = 0; i < 8; i++)
                g_calibration.scale[i] = 1.0f / rn;
        }

        g_calibration.valid = true;
        g_calibration.magic = CALIBRATION_MAGIC;
        return classifier_save_calibration();
    }

    return true;
}

/* Core classification function */
classify_result_t classifier_classify(const float raw_feature[8], float liftoff_mm)
{
    classify_result_t result;
    memset(&result, 0, sizeof(result));
    result.alloy_index = -1;

    if (!classifier_is_calibrated()) {
        result.uncertain = true;
        return result;
    }

    /* Step 1: Subtract baseline (open circuit reference) */
    float compensated[8];
    vec_sub(compensated, raw_feature, g_calibration.baseline);

    /* Step 2: Apply scale */
    for (int i = 0; i < 8; i++)
        compensated[i] *= g_calibration.scale[i];

    /* Step 3: Project out lift-off component
     * Remove the component along the lift-off direction:
     *   v' = v - (v . lift_dir) * lift_dir */
    float proj = vec_dot(compensated, g_calibration.liftoff_dir);
    for (int i = 0; i < 8; i++)
        compensated[i] -= proj * g_calibration.liftoff_dir[i];

    /* Step 4: Normalize the feature vector */
    vec_normalize(compensated);
    memcpy(result.feature, compensated, sizeof(float) * 8);
    result.liftoff_mm = liftoff_mm;

    /* Step 5: Edge detection via cross-frequency consistency
     * If the measurement is near an edge, the low-frequency and
     * high-frequency components will be inconsistent. We check this
     * by comparing the ratio of low-freq to high-freq magnitude
     * against what the nearest alloys predict. */
    float low_mag = sqrtf(compensated[0]*compensated[0] + compensated[1]*compensated[1]
                        + compensated[2]*compensated[2] + compensated[3]*compensated[3]);
    float high_mag = sqrtf(compensated[4]*compensated[4] + compensated[5]*compensated[5]
                         + compensated[6]*compensated[6] + compensated[7]*compensated[7]);
    float ratio = (high_mag > 1e-6f) ? (low_mag / high_mag) : 0.0f;

    /* Step 6: Compute distances to all alloys in database */
    float distances[ALLOY_DB_COUNT];
    for (int i = 0; i < ALLOY_DB_COUNT; i++) {
        distances[i] = classifier_distance(compensated, alloy_database[i].feature);
    }

    /* Step 7: Find k=5 nearest neighbors (simple selection sort for k elements) */
    int nn_idx[KNN_K];
    float nn_dist[KNN_K];

    for (int k = 0; k < KNN_K; k++) {
        nn_idx[k] = -1;
        nn_dist[k] = 1e30f;
        for (int i = 0; i < ALLOY_DB_COUNT; i++) {
            /* Skip if already selected */
            bool already = false;
            for (int j = 0; j < k; j++) {
                if (nn_idx[j] == i) { already = true; break; }
            }
            if (already) continue;

            if (distances[i] < nn_dist[k]) {
                nn_dist[k] = distances[i];
                nn_idx[k] = i;
            }
        }
    }

    /* Step 8: Weighted k-NN voting
     * Weight = 1/distance. Sum weights per alloy class.
     * Confidence = winning class weight / total weight. */
    float total_weight = 0.0f;
    float class_weights[ALLOY_DB_COUNT];
    memset(class_weights, 0, sizeof(class_weights));

    for (int k = 0; k < KNN_K; k++) {
        if (nn_idx[k] >= 0 && nn_dist[k] > 1e-9f) {
            float w = 1.0f / nn_dist[k];
            class_weights[nn_idx[k]] += w;
            total_weight += w;
        }
    }

    /* Find the winning class */
    float best_weight = 0.0f;
    int best_idx = -1;
    for (int i = 0; i < ALLOY_DB_COUNT; i++) {
        if (class_weights[i] > best_weight) {
            best_weight = class_weights[i];
            best_idx = i;
        }
    }

    result.alloy_index = best_idx;
    result.distance = (best_idx >= 0) ? distances[best_idx] : 1e30f;
    result.confidence = (total_weight > 0.0f) ? (best_weight / total_weight) : 0.0f;

    /* Fill alternatives (next nearest different alloys) */
    int alt_count = 0;
    for (int k = 0; k < KNN_K && alt_count < MAX_ALTERNATIVES; k++) {
        if (nn_idx[k] >= 0 && nn_idx[k] != best_idx) {
            result.alternatives[alt_count] = nn_idx[k];
            result.alt_distances[alt_count] = nn_dist[k];
            alt_count++;
        }
    }
    result.alt_count = alt_count;

    /* Step 9: Edge detection — compare measured ratio to winner's ratio */
    if (best_idx >= 0) {
        const alloy_entry_t *winner = &alloy_database[best_idx];
        float w_low = sqrtf(winner->feature[0]*winner->feature[0]
                           + winner->feature[1]*winner->feature[1]
                           + winner->feature[2]*winner->feature[2]
                           + winner->feature[3]*winner->feature[3]);
        float w_high = sqrtf(winner->feature[4]*winner->feature[4]
                            + winner->feature[5]*winner->feature[5]
                            + winner->feature[6]*winner->feature[6]
                            + winner->feature[7]*winner->feature[7]);
        float w_ratio = (w_high > 1e-6f) ? (w_low / w_high) : 0.0f;

        /* If the measured ratio deviates >50% from expected, flag edge */
        if (w_ratio > 1e-6f) {
            float ratio_dev = fabsf(ratio - w_ratio) / w_ratio;
            if (ratio_dev > 0.5f)
                result.edge_warning = true;
        }
    }

    /* Step 10: Uncertainty flag */
    result.uncertain = (result.confidence < CONFIDENCE_MEDIUM);

    return result;
}

bool classifier_train_custom(const char *name, const float measurements[][8],
                             int count)
{
    if (count < 1 || !name || strlen(name) >= ALLOY_NAME_MAX)
        return false;

    /* Average the measurements into a feature vector, then compensate
     * the same way as in classify(). Store as a new alloy entry.
     * In a full implementation, this writes to flash and updates
     * the in-memory database. */
    float avg[8];
    memset(avg, 0, sizeof(avg));
    for (int m = 0; m < count; m++)
        for (int i = 0; i < 8; i++)
            avg[i] += measurements[m][i];
    for (int i = 0; i < 8; i++)
        avg[i] /= (float)count;

    /* Subtract baseline and normalize */
    vec_sub(avg, avg, g_calibration.baseline);
    for (int i = 0; i < 8; i++)
        avg[i] *= g_calibration.scale[i];

    /* Project out lift-off */
    float proj = vec_dot(avg, g_calibration.liftoff_dir);
    for (int i = 0; i < 8; i++)
        avg[i] -= proj * g_calibration.liftoff_dir[i];
    vec_normalize(avg);

    /* In production: write this as a new entry to flash at the next
     * available slot. For now, we just return success. */
    (void)avg; /* Would be stored to flash here */
    return true;
}