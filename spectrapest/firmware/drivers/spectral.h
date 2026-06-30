/*
 * drivers/spectral.h — Multispectral analysis for pest identification
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 */

#ifndef SPECTRAL_H
#define SPECTRAL_H

#include <stdint.h>
#include "../board.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPECTRAL_FEATURE_DIM  36
#define SPECTRAL_BANDS        BAND_COUNT  /* 6 bands */
#define SPECTRAL_IMG_W        128
#define SPECTRAL_IMG_H        96
#define SPECTRAL_IMG_SIZE     (SPECTRAL_IMG_W * SPECTRAL_IMG_H)

typedef struct {
    /* Per-band statistics (6 bands × 4 stats = 24 features) */
    float band_mean[SPECTRAL_BANDS];
    float band_variance[SPECTRAL_BANDS];
    float band_skew[SPECTRAL_BANDS];
    float band_kurtosis[SPECTRAL_BANDS];

    /* Vegetation indices (12 features) */
    float ndvi;           /* (NIR1 - Red) / (NIR1 + Red) = (810-660)/(810+660) */
    float ndre;           /* (NIR1 - RedEdge) / (NIR1 + RedEdge) = (810-740)/(810+740) */
    float gndvi;          /* (NIR1 - Green) / (NIR1 + Green) */
    float evi;            /* Enhanced vegetation index */
    float savi;           /* Soil-adjusted vegetation index */
    float arvi;           /* Atmospherically resistant vegetation index */
    float pri;            /* Photochemical reflectance index (530/450) */
    float sipi;           /* Structural independent pigment index */
    float psri;           /* Plant senescence reflectance index */
    float red_edge_ratio; /* 740/660 ratio */
    float damage_area_pct;  /* Percentage of pixels showing damage signature */
    float chlorosis_index;  /* Chlorosis severity 0-1 */

    /* Derived from band ratios */
    float ratio_blue_green;
    float ratio_red_nir;
    float ratio_nir1_nir2;
    float ratio_rededge_red;
    float texture_contrast;
    float texture_homogeneity;
    float texture_energy;
    float texture_correlation;
} spectral_features_t;

int spectral_init(void);
int spectral_capture_band(filter_band_t band, uint16_t *out_img);
int spectral_extract_features(const uint16_t *band_images[SPECTRAL_BANDS],
                                spectral_features_t *out_features);
int spectral_capture_all(uint16_t band_images[SPECTRAL_BANDS][SPECTRAL_IMG_SIZE]);
void spectral_filter_wheel_home(void);
int  spectral_filter_wheel_move(filter_band_t band);

/* GLCM texture features */
void spectral_compute_glcm(const uint16_t *img, uint32_t w, uint32_t h,
                            float *contrast, float *homogeneity,
                            float *energy, float *correlation);

#ifdef __cplusplus
}
#endif

#endif /* SPECTRAL_H */