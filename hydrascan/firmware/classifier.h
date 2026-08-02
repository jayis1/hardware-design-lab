/*
 * classifier.h — PCA projection + Mahalanobis distance + mixture interp.
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_CLASSIFIER_H
#define HYDRASCAN_CLASSIFIER_H

#include "../board.h"

#define NAME_LEN 28

typedef struct {
    uint16_t class_id;
    char     name[NAME_LEN];
    /* Centroid in PCA space. */
    int32_t  centroid[FEATURE_DIM_PCA];   /* Q16.16 fixed point        */
    int32_t  variance[FEATURE_DIM_PCA];   /* Q16.16 variance          */
} liquid_class_t;

typedef struct {
    uint32_t         n_classes;
    liquid_class_t   classes[MAX_LIQUID_CLASSES];
} library_t;

typedef struct {
    uint16_t class_id;
    char     name[NAME_LEN];
    float    confidence;          /* 0..1                          */
    uint8_t  adulterant;          /* 1 if blend between two classes*/
    float    adulterant_ratio;   /* 0..1 along the pair line       */
    int32_t  pca_z[FEATURE_DIM_PCA];
} classify_result_t;

/* Build the 49-dim raw feature vector from the two sweeps + temp. */
void classifier_build_feature(float raw[FEATURE_DIM_RAW],
                              const float opt[OPTICAL_WAVELENGHS],
                              const eis_point_t eis[EIS_FREQ_POINTS],
                              float temp_c);

/* Whitening + PCA project raw→z (16-dim). The PCA matrix is fixed and
 * embedded in the firmware; temperature normalisation is applied here. */
void classifier_project(const float raw[FEATURE_DIM_RAW],
                        int32_t z[FEATURE_DIM_PCA]);

/* Classify a projected point against the library. */
hydra_err_t classifier_classify(const library_t *lib,
                                const int32_t z[FEATURE_DIM_PCA],
                                classify_result_t *out);

/* Fixed-point Q16.16 helpers. */
int32_t q16_from_float(float x);
float   q16_to_float(int32_t q);

#endif