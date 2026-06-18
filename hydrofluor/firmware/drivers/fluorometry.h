/*
 * fluorometry.h — HydroFluor fluorescence unmixing & concentration model
 * Author: jayis1
 * License: MIT
 */
#ifndef FLUOROMETRY_H
#define FLUOROMETRY_H

#include "board.h"
#include "photodiode.h"
#include <stdint.h>

/* Analyte IDs (must match the app's order) */
typedef enum {
    ANALYTE_CDOM   = 0,   /* ppb QSU   */
    ANALYTE_CHLA   = 1,   /* μg/L      */
    ANALYTE_PHYC   = 2,   /* μg/L      */
    ANALYTE_OIL    = 3,   /* ppb       */
    ANALYTE_TURB   = 4,   /* NTU       */
    ANALYTE_COUNT
} analyte_t;

typedef struct {
    uint16_t cdom_ppb;
    uint16_t chla_ugl;
    uint16_t phyc_ugl;
    uint16_t oil_ppb;
    uint16_t turb_ntu_x100;   /* 0.01 NTU units */
    uint16_t flags;
} analyte_result_t;

/* PLS model coefficients stored in QSPI flash (float16 quantized). */
typedef struct {
    int16_t w[PLS_LATENT_VARS][PLS_FEATURES];   /* projection weights */
    int16_t b[PLS_LATENT_VARS];                 /* latent scores offsets */
    int16_t beta[PLS_LATENT_VARS];              /* regression coefficients */
    int16_t mu_x[PLS_FEATURES];                 /* mean of features   */
    int16_t mu_y;                                /* mean of analyte    */
    int16_t scale;                              /* output scale (1e-3) */
} pls_model_t;

/* Load PLS models from QSPI flash into RAM. Returns 0 on success. */
int  fluorometry_load_models(void);

/* Run the unmixing model on an acquisition to produce analyte concentrations.
 * Applies temperature compensation (in 0.01°C) and reference normalization. */
void fluorometry_unmix(const acquisition_t *acq,
                        int16_t temp_c01,
                        analyte_result_t *out);

/* Apply a calibration (reference_concentration, raw_vector) pair to the
 * on-device model. Used by the BLE calibrate characteristic. */
int  fluorometry_calibrate(analyte_t which, uint16_t ref_value,
                            const int32_t raw[PLS_FEATURES]);

/* Temperature compensation coefficient per analyte (ppm per °C, quadratic). */
void fluorometry_set_temp_comp(analyte_t a, float k1, float k2);

/* Convert an analyte result to a human-readable string for the console. */
void fluorometry_format(const analyte_result_t *r, char *buf, size_t len);

#endif /* FLUOROMETRY_H */