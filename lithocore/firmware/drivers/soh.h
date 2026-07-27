/*
 * soh.h — State-of-Health scoring and degradation mode classification
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_SOH_H
#define LITHOCORE_SOH_H

#include <stdint.h>
#include "randles.h"
#include "eis_sweep.h"
#include "../board.h"

/* Degradation modes */
typedef enum {
    DEGRAD_HEALTHY       = 0,
    DEGRAD_SEI_GROWTH    = 1,
    DEGRAD_LI_PLATING    = 2,
    DEGRAD_DRYOUT        = 3,
    DEGRAD_INTERNAL_SHORT = 4,
    DEGRAD_UNKNOWN       = 5,
} degradation_mode_t;

/* Quality verdict (derived from SoH score) */
typedef enum {
    VERDICT_EXCELLENT  = 0,  /* SoH >= 85 */
    VERDICT_GOOD       = 1,  /* SoH >= 70 */
    VERDICT_FAIR       = 2,  /* SoH >= 50 */
    VERDICT_POOR       = 3,  /* SoH >= 30 */
    VERDICT_REPLACE    = 4,  /* SoH < 30 */
} quality_verdict_t;

/* Complete SoH result — stored in flash and sent to the app */
typedef struct {
    /* Raw measurements */
    uint16_t ocv_mv;                    /* open-circuit voltage mV */
    uint16_t temp_dc;                   /* temperature deci-degC (×10) */
    uint16_t dcir_mohm;                 /* DC internal resistance mΩ */
    int32_t  self_discharge_uv_per_min; /* self-discharge rate µV/min */
    uint8_t  chemistry_idx;             /* index into chemistry_table */

    /* EIS sweep data */
    eis_sweep_data_t sweep_data;

    /* CNLS fit results */
    randles_params_t randles;
    uint8_t          fit_valid;

    /* Computed results */
    uint8_t              soh_score;        /* 0-100 */
    degradation_mode_t   degradation;      /* classified mode */
    quality_verdict_t    verdict;          /* quality verdict */
    double               chisq;            /* fit quality (lower = better) */

    /* Timestamp */
    uint32_t timestamp;   /* RTC time or tick count */
} soh_result_t;

/* API */
void soh_compute(soh_result_t *result, const chemistry_baseline_t *baseline);
const char *soh_mode_name(degradation_mode_t mode);
const char *soh_verdict_name(quality_verdict_t verdict);
const char *soh_chemistry_name(uint8_t idx);

/* k-NN classifier: compares fitted parameters against a reference
 * database of known degradation states. Returns the classified mode. */
degradation_mode_t soh_classify(const randles_params_t *fitted,
                                const chemistry_baseline_t *baseline,
                                int32_t dcir_mohm,
                                int32_t self_discharge_uv_per_min);

#endif /* LITHOCORE_SOH_H */