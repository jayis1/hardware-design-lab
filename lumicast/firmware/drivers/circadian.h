/*
 * circadian.h — Circadian photometry calculations
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_CIRCADIAN_H
#define LUMICAST_CIRCADIAN_H

#include <stdint.h>
#include "spectrometer.h"

/* Comprehensive photometric/circadian result. */
typedef struct {
    /* Photometric quantities. */
    float illuminance_lux;          /* photopic illuminance (lux) */
    float cct_k;                    /* correlated color temperature (K) */
    float duv;                      /* distance from Planckian locus */
    float x, y;                     /* CIE 1931 chromaticity */
    float u_prime, v_prime;         /* CIE 1976 u'v' */
    float cri_ra;                   /* estimated color rendering index */
    float tm_30_rfg;                /* estimated TM-30 Rf (gamut) */
    float tm_30_rgs;                /* estimated TM-30 Rg (saturation) */

    /* Circadian quantities (per CIE S 026:2018). */
    float melanopic_edi;            /* melanopic equivalent daylight lux */
    float melanopic_er;             /* melanopic equivalent daylight ratio */
    float photopic_lux;            /* photopic lux (same as illuminance) */
    float cyanopic_edi;            /* S-cone-opic equivalent daylight lux */
    float circadian_stimulus;       /* Rea-Figueiro CS (0..0.7) */
    float circadian_lightCLA;      /* Rea CLA 2.0 model */
    float equivalent_melanopic_lux;/* EML (WELL v2 term) */

    /* Time-of-day circadian state. */
    int8_t circadian_phase;         /* -1=biological night, 0=neutral, 1=day */
    uint32_t timestamp_ms;
} circadian_result_t;

void circadian_compute(const spectral_sample_t *spec,
                       circadian_result_t *out);
float circadian_melanopic_edi(const spectral_sample_t *spec);
float circadian_illuminance(const spectral_sample_t *spec);
void circadian_cct_duv(const spectral_sample_t *spec,
                       float *cct, float *duv, float *x, float *y);
float circadian_rea_CLA(const spectral_sample_t *spec);
float circadian_rea_CS(float CLA);
int8_t circadian_phase_estimate(uint32_t local_time_s);

#endif /* LUMICAST_CIRCADIAN_H */