/*
 * drivers/spectral.c — Multispectral analysis for pest identification
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Controls the 6-position filter wheel (driven by STSPIN220 stepper motor)
 * to capture multispectral images at 450, 530, 660, 740, 810, and 850 nm.
 * For each band, the IMX519 captures a frame, which is downsampled to
 * 128×96 RGB565 and processed to extract spectral features.
 *
 * The spectral feature vector (36 dimensions) captures:
 *   - Per-band statistics (mean, variance, skew, kurtosis): 24 features
 *   - Vegetation indices (NDVI, NDRE, GNDVI, EVI, SAVI, ARVI, PRI, SIPI,
 *     PSRI, red-edge ratio): 10 features
 *   - Damage area percentage: 1 feature
 *   - Chlorosis index: 1 feature
 *   - Texture features (GLCM contrast, homogeneity, energy, correlation): 4 features
 *
 * Vegetation indices are the key discriminants for pest damage:
 *   - NDVI drops when chlorophyll is degraded (aphid/chewing damage)
 *   - NDRE detects early stress before visible symptoms (red-edge shift)
 *   - PRI (530/450) changes with xanthophyll cycle activity (stress response)
 *   - Damage area % from thresholding: red band < 0.3 * NIR band indicates
 *     chlorotic spots; brown necrotic tissue has low reflectance in all bands
 *
 * The filter wheel homing uses an optical sensor on the HOME pin. After
 * homing, the stepper moves 200 microsteps per band position. The STSPIN220
 * is configured for 1/8 microstepping (1600 steps/rev) with a 48:1 gear ratio,
 * giving 76800 steps/rev at the wheel — 12800 steps per 60° band position.
 */

#include "spectral.h"
#include "imx519.h"
#include "../registers.h"
#include "../board.h"
#include <math.h>
#include <string.h>

/* ----------------------------------------------------------------- *
 *  Filter wheel stepper control
 * ----------------------------------------------------------------- */
#define STEP_MICROSTEPS  8
#define STEP_FULL_STEPS  200
#define STEP_STEPS_PER_REV (STEP_FULL_STEPS * STEP_MICROSTEPS)
#define WHEEL_GEAR_RATIO 48
#define WHEEL_STEPS_PER_BAND (STEP_STEPS_PER_REV * WHEEL_GEAR_RATIO / BAND_COUNT)

/* Stepper step pulse timing: 20 µs high, 20 µs low = 25 kHz max */
#define STEP_PULSE_HIGH_US  20
#define STEP_PULSE_LOW_US   20

static filter_band_t g_current_band = BAND_450;
static uint8_t g_wheel_homed = 0;

/* ----------------------------------------------------------------- *
 *  Initialize spectral subsystem
 * ----------------------------------------------------------------- */
int spectral_init(void) {
    /* Configure stepper motor pins */
    gpio_set_mode(STEP_STEP_PORT, STEP_STEP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(STEP_DIR_PORT, STEP_DIR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(STEP_EN_PORT, STEP_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(STEP_MS1_PORT, STEP_MS1_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(STEP_MS2_PORT, STEP_MS2_PIN, GPIO_MODE_OUTPUT);
    gpio_set_mode(STEP_HOME_PORT, STEP_HOME_PIN, GPIO_MODE_INPUT);
    gpio_set_pupd(STEP_HOME_PORT, STEP_HOME_PIN, GPIO_PUPD_UP);

    /* Set microstepping: 1/8 (MS1=1, MS2=0) */
    gpio_write(STEP_MS1_PORT, STEP_MS1_PIN, 1);
    gpio_write(STEP_MS2_PORT, STEP_MS2_PIN, 0);

    /* Disable stepper initially */
    gpio_write(STEP_EN_PORT, STEP_EN_PIN, 1);  /* Active low */

    /* Home the filter wheel */
    spectral_filter_wheel_home();

    /* Initialize camera */
    if (imx519_init() < 0) return -1;

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Step pulse generation (bit-banged)
 *  Uses TIM2 for precise timing.
 * ----------------------------------------------------------------- */
static void step_pulse(void) {
    gpio_write(STEP_STEP_PORT, STEP_STEP_PIN, 1);
    /* 20 µs delay using busy-wait (at 480 MHz, ~9600 cycles) */
    for (volatile int i = 0; i < 960; i++) __asm__("nop");
    gpio_write(STEP_STEP_PORT, STEP_STEP_PIN, 0);
    for (volatile int i = 0; i < 960; i++) __asm__("nop");
}

/* ----------------------------------------------------------------- *
 *  Home the filter wheel using optical sensor
 * ----------------------------------------------------------------- */
void spectral_filter_wheel_home(void) {
    /* Enable stepper */
    gpio_write(STEP_EN_PORT, STEP_EN_PIN, 0);

    /* Set direction to clockwise */
    gpio_write(STEP_DIR_PORT, STEP_DIR_PIN, 1);

    /* Step until home sensor is triggered, or max steps reached */
    uint32_t max_steps = WHEEL_STEPS_PER_BAND * BAND_COUNT * 2;
    for (uint32_t i = 0; i < max_steps; i++) {
        if (!gpio_read(STEP_HOME_PORT, STEP_HOME_PIN)) {
            /* Home sensor triggered (active low) */
            break;
        }
        step_pulse();
    }

    /* Back off 50 steps to ensure we're off the sensor */
    gpio_write(STEP_DIR_PORT, STEP_DIR_PIN, 0);
    for (int i = 0; i < 50; i++) {
        step_pulse();
    }

    g_current_band = BAND_450;
    g_wheel_homed = 1;

    /* Disable stepper to save power */
    gpio_write(STEP_EN_PORT, STEP_EN_PIN, 1);
}

/* ----------------------------------------------------------------- *
 *  Move filter wheel to a specific band
 * ----------------------------------------------------------------- */
int spectral_filter_wheel_move(filter_band_t band) {
    if (band >= BAND_COUNT) return -1;
    if (!g_wheel_homed) spectral_filter_wheel_home();

    /* Calculate steps needed */
    int32_t diff = (int32_t)band - (int32_t)g_current_band;
    if (diff < 0) diff += BAND_COUNT;

    uint32_t steps = (uint32_t)diff * WHEEL_STEPS_PER_BAND;

    /* Enable stepper */
    gpio_write(STEP_EN_PORT, STEP_EN_PIN, 0);
    board_delay_ms(5);

    /* Set direction */
    gpio_write(STEP_DIR_PORT, STEP_DIR_PIN, 1);

    /* Step to target position */
    for (uint32_t i = 0; i < steps; i++) {
        step_pulse();
    }

    g_current_band = band;

    /* Allow filter to settle (10 ms) */
    board_delay_ms(10);

    /* Disable stepper to save power */
    gpio_write(STEP_EN_PORT, STEP_EN_PIN, 1);

    return 0;
}

/* ----------------------------------------------------------------- *
 *  Capture a single band image
 * ----------------------------------------------------------------- */
int spectral_capture_band(filter_band_t band, uint16_t *out_img) {
    if (!out_img || band >= BAND_COUNT) return -1;

    /* Move filter wheel to the target band */
    if (spectral_filter_wheel_move(band) < 0) return -1;

    /* Determine if we need artificial illumination */
    uint16_t ambient_light = 0;  /* Would read from VEML7700 */
    uint8_t  need_strobe = (ambient_light < 500) ? 1 : 0;

    if (need_strobe) {
        imx519_set_strobe(1);
        board_delay_ms(5);
    }

    /* Capture frame (downsampled to 128×96 via DMA line accumulator) */
    int result = imx519_capture_bayer(out_img, SPECTRAL_IMG_SIZE);

    if (need_strobe) {
        imx519_set_strobe(0);
    }

    return result;
}

/* ----------------------------------------------------------------- *
 *  Capture all 6 bands sequentially
 * ----------------------------------------------------------------- */
int spectral_capture_all(uint16_t band_images[SPECTRAL_BANDS][SPECTRAL_IMG_SIZE]) {
    for (int b = 0; b < SPECTRAL_BANDS; b++) {
        if (spectral_capture_band((filter_band_t)b, band_images[b]) < 0) {
            return -1;
        }
    }
    return 0;
}

/* ----------------------------------------------------------------- *
 *  Compute statistical moments of a band image
 * ----------------------------------------------------------------- */
static void compute_band_stats(const uint16_t *img, uint32_t n,
                                float *mean, float *variance,
                                float *skew, float *kurtosis) {
    if (n == 0) { *mean = *variance = *skew = *kurtosis = 0; return; }

    /* Mean */
    double sum = 0;
    for (uint32_t i = 0; i < n; i++) {
        sum += (double)img[i];
    }
    *mean = (float)(sum / n);

    /* Variance */
    double sq_diff = 0, cb_diff = 0, qt_diff = 0;
    for (uint32_t i = 0; i < n; i++) {
        double d = (double)img[i] - *mean;
        sq_diff += d * d;
        cb_diff += d * d * d;
        qt_diff += d * d * d * d;
    }
    *variance = (float)(sq_diff / n);
    float std = sqrtf(*variance);
    if (std > 0.001f) {
        *skew = (float)(cb_diff / n) / (std * std * std);
        *kurtosis = (float)(qt_diff / n) / (*variance * *variance) - 3.0f;
    } else {
        *skew = 0;
        *kurtosis = 0;
    }
}

/* ----------------------------------------------------------------- *
 *  Compute GLCM (Gray-Level Co-occurrence Matrix) texture features
 *  Uses 16 gray levels (4-bit) for a compact 16×16 GLCM.
 * ----------------------------------------------------------------- */
void spectral_compute_glcm(const uint16_t *img, uint32_t w, uint32_t h,
                            float *contrast, float *homogeneity,
                            float *energy, float *correlation) {
    /* Quantize to 16 levels */
    float glcm[16][16];
    memset(glcm, 0, sizeof(glcm));
    uint32_t count = 0;

    /* Build GLCM with offset (1, 0) — horizontal co-occurrence */
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w - 1; x++) {
            uint8_t a = (img[y * w + x] >> 4) & 0x0F;
            uint8_t b = (img[y * w + x + 1] >> 4) & 0x0F;
            glcm[a][b] += 1.0f;
            glcm[b][a] += 1.0f;  /* Symmetric */
            count += 2;
        }
    }

    /* Normalize */
    if (count > 0) {
        for (int i = 0; i < 16; i++)
            for (int j = 0; j < 16; j++)
                glcm[i][j] /= (float)count;
    }

    /* Compute features */
    float mu_i = 0, mu_j = 0, sigma_i = 0, sigma_j = 0;
    *contrast = *homogeneity = *energy = *correlation = 0;

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            float p = glcm[i][j];
            *contrast += (float)((i - j) * (i - j)) * p;
            *homogeneity += p / (1.0f + (float)(i - j) * (i - j));
            *energy += p * p;
            mu_i += (float)i * p;
            mu_j += (float)j * p;
        }
    }

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            sigma_i += glcm[i][j] * (float)i * (float)i;
            sigma_j += glcm[i][j] * (float)j * (float)j;
        }
    }
    sigma_i = sqrtf(sigma_i - mu_i * mu_i);
    sigma_j = sqrtf(sigma_j - mu_j * mu_j);

    if (sigma_i > 0.001f && sigma_j > 0.001f) {
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 16; j++) {
                *correlation += ((float)i - mu_i) * ((float)j - mu_j) * glcm[i][j] / (sigma_i * sigma_j);
            }
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Extract spectral features from 6-band image set
 * ----------------------------------------------------------------- */
int spectral_extract_features(const uint16_t *band_images[SPECTRAL_BANDS],
                                spectral_features_t *out) {
    if (!band_images || !out) return -1;

    uint32_t n = SPECTRAL_IMG_SIZE;

    /* Compute per-band statistics */
    for (int b = 0; b < SPECTRAL_BANDS; b++) {
        if (!band_images[b]) return -1;
        compute_band_stats(band_images[b], n,
                           &out->band_mean[b], &out->band_variance[b],
                           &out->band_skew[b], &out->band_kurtosis[b]);
    }

    /* Extract individual band means for index computation */
    float blue    = out->band_mean[BAND_450];
    float green   = out->band_mean[BAND_530];
    float red     = out->band_mean[BAND_660];
    float rededge = out->band_mean[BAND_740];
    float nir1    = out->band_mean[BAND_810];
    float nir2    = out->band_mean[BAND_850];

    /* Vegetation indices */
    out->ndvi = (nir1 + red > 0.001f) ? (nir1 - red) / (nir1 + red) : 0;
    out->ndre = (nir1 + rededge > 0.001f) ? (nir1 - rededge) / (nir1 + rededge) : 0;
    out->gndvi = (nir1 + green > 0.001f) ? (nir1 - green) / (nir1 + green) : 0;
    out->evi = (nir1 + 6*red - 7.5*blue > 0.001f)
              ? 2.5f * (nir1 - red) / (nir1 + 6*red - 7.5f*blue) : 0;
    float L = 0.5f;
    out->savi = (nir1 + red + L > 0.001f) ? ((nir1 - red) * (1 + L)) / (nir1 + red + L) : 0;
    out->arvi = (nir1 + red - 2*(red - blue) > 0.001f)
               ? (nir1 - (red - 2*(red - blue))) / (nir1 + (red - 2*(red - blue))) : 0;
    out->pri = (green + blue > 0.001f) ? (green - blue) / (green + blue) : 0;
    out->sipi = (nir1 + red > 0.001f) ? (nir1 - blue) / (nir1 + red) : 0;
    out->psri = (red > 0.001f) ? (red - green) / rededge : 0;
    out->red_edge_ratio = (red > 0.001f) ? rededge / red : 0;

    /* Damage area: count pixels where red reflectance is abnormally low
     * relative to NIR (indicating chlorosis or necrosis) */
    uint32_t damage_count = 0;
    for (uint32_t i = 0; i < n; i++) {
        float p_red = band_images[BAND_660][i];
        float p_nir = band_images[BAND_810][i];
        if (p_nir > 0.001f && p_red / p_nir < 0.3f) {
            damage_count++;
        }
    }
    out->damage_area_pct = 100.0f * (float)damage_count / (float)n;

    /* Chlorosis index: deviation of green/red ratio from healthy baseline */
    float gr_ratio = (red > 0.001f) ? green / red : 1.0f;
    out->chlorosis_index = (gr_ratio > 1.2f) ? (gr_ratio - 1.0f) : 0;
    if (out->chlorosis_index > 1.0f) out->chlorosis_index = 1.0f;

    /* Band ratios */
    out->ratio_blue_green = (green > 0.001f) ? blue / green : 0;
    out->ratio_red_nir = (nir1 > 0.001f) ? red / nir1 : 0;
    out->ratio_nir1_nir2 = (nir2 > 0.001f) ? nir1 / nir2 : 0;
    out->ratio_rededge_red = (red > 0.001f) ? rededge / red : 0;

    /* Texture features from NIR1 band (most informative for damage morphology) */
    spectral_compute_glcm(band_images[BAND_810], SPECTRAL_IMG_W, SPECTRAL_IMG_H,
                          &out->texture_contrast, &out->texture_homogeneity,
                          &out->texture_energy, &out->texture_correlation);

    return 0;
}