/*
 * fusion.h — Sensor Fusion & TinyML Inference Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_FUSION_H
#define FERMENTIQ_FUSION_H

#include "board.h"

typedef struct {
    fermentation_phase_t phase;
    float abv_estimate;     /* % ABV                    */
    float attenuation;      /* apparent attenuation %   */
    int   spoilage_risk;    /* 0-100 risk score         */
    float health_score;     /* 0-100 fermentation health */
} fusion_result_t;

/* API */
int fusion_init(void);
float fusion_predict_biomass(const impedance_data_t *imp);
void fusion_infer(const impedance_data_t *imp,
                  const co2_data_t *co2,
                  const ph_data_t *ph,
                  const temp_data_t *temp,
                  const acoustic_data_t *ac,
                  const fusion_data_t *prev,
                  float co2_total_mol,
                  uint32_t batch_age_hours,
                  fusion_result_t *result);

#endif /* FERMENTIQ_FUSION_H */