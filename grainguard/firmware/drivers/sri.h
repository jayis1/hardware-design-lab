/*
 * sri.h — Spoilage Risk Index fusion (header)
 * Author: jayis1  Copyright (C) 2026 jayis1  License: GPL-2.0
 */
#ifndef GRAINGUARD_SRI_H
#define GRAINGUARD_SRI_H

#include <stdint.h>
#include "co2.h"
#include "temp.h"
#include "humid.h"
#include "acoustic.h"

typedef struct {
    uint8_t  sri;            /* 0-100 composite score */
    uint8_t  co2_contribution;     /* 0-35 */
    uint8_t  temp_grad_contribution; /* 0-25 */
    uint8_t  temp_abs_contribution;  /* 0-15 */
    uint8_t  emc_contribution;       /* 0-15 */
    uint8_t  acoustic_contribution;  /* 0-10 */
    uint8_t  alert_level;   /* 0=OK, 1=Caution, 2=Critical */
} sri_result_t;

/* Compute the Spoilage Risk Index from all sensor inputs. */
void sri_compute(sri_result_t *sri,
                 const co2_meas_t *co2,
                 const temp_profile_t *temp,
                 const humid_meas_t *humid,
                 const acoustic_result_t *acoustic,
                 uint8_t grain_type,
                 int16_t safe_mc_x1000,
                 uint8_t caution_thresh,
                 uint8_t critical_thresh);

#endif /* GRAINGUARD_SRI_H */