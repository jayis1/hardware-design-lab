/*
 * breath_state.c — Breath sample state machine and quality validation
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 *
 * Provides higher-level breath validation and state machine helpers
 * that complement the main firmware state machine.
 */

#include "breath_state.h"
#include "sensor_array.h"
#include "../board.h"

/* ========================================================================
 * Breath quality validation
 * ========================================================================
 *
 * Validates that a breath sample represents true alveolar (deep lung)
 * air and not dead-space air or environmental contamination.
 *
 * Primary validation: CO2 level must exceed 3.5% (35000 ppm).
 * Secondary: sensor response amplitudes must show meaningful change.
 * ========================================================================
 */

uint8_t breath_validate(const sensor_raw_t *samples, uint32_t count,
                        calib_data_t *calib) {
    if (count < 10) {
        return BREATH_RETRY;
    }

    /* --- CO2 Validation --- */
    /* Find peak CO2 during the sample window */
    uint16_t max_co2 = 0;
    uint16_t min_co2 = 0xFFFF;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].scd41_co2 > max_co2) max_co2 = samples[i].scd41_co2;
        if (samples[i].scd41_co2 < min_co2) min_co2 = samples[i].scd41_co2;
    }

    /* CO2 must exceed alveolar threshold (3.5%) */
    if (max_co2 < CO2_ALVEOLAR_MIN) {
        return BREATH_DEAD_SPACE;
    }

    /* CO2 must show a clear rise (exhalation signature) */
    uint16_t co2_rise = max_co2 - min_co2;
    if (co2_rise < 10000) {  /* Must rise by at least 1% (10000 ppm) */
        return BREATH_DEAD_SPACE;
    }

    /* --- Sensor Response Validation --- */
    /* At least 2 sensor channels must show >10% change from baseline */
    int responding_channels = 0;
    for (int ch = 0; ch < NUM_SENSORS; ch++) {
        float baseline = sensor_array_get_channel(&samples[0], ch);
        float max_val = baseline;

        for (uint32_t i = 1; i < count; i++) {
            float v = sensor_array_get_channel(&samples[i], ch);
            if (v > max_val) max_val = v;
        }

        /* Channel responded if >10% increase from baseline */
        if (baseline > 0 && max_val > baseline * 1.10f) {
            responding_channels++;
        }
    }

    if (responding_channels < 2) {
        return BREATH_CONTAMINATED;
    }

    /* --- Temporal Consistency Check --- */
    /* The CO2 curve should show a characteristic rise-plateau pattern.
     * Verify that CO2 rises in the first half and stabilizes in the second. */
    uint32_t mid_point = count / 2;
    uint16_t co2_first_half_max = 0;
    uint16_t co2_second_half_avg = 0;

    for (uint32_t i = 0; i < mid_point; i++) {
        if (samples[i].scd41_co2 > co2_first_half_max) {
            co2_first_half_max = samples[i].scd41_co2;
        }
    }

    uint32_t second_count = count - mid_point;
    uint32_t sum = 0;
    for (uint32_t i = mid_point; i < count; i++) {
        sum += samples[i].scd41_co2;
    }
    co2_second_half_avg = (uint16_t)(sum / second_count);

    /* Second half average should be near the first half peak (plateau) */
    if (co2_second_half_avg < co2_first_half_max * 0.7f) {
        /* CO2 dropped too quickly — might be a short breath */
        return BREATH_RETRY;
    }

    return BREATH_VALID;
}

/* ========================================================================
 * Estimate breath markers from sample data
 * ========================================================================
 */

void breath_estimate(const sensor_raw_t *samples, uint32_t count,
                     calib_data_t *calib, feature_vector_t *features,
                     breath_result_t *result) {
    memset(result, 0, sizeof(*result));

    if (count == 0 || !calib || !features) return;

    /* --- Acetone (from BME688 #1) --- */
    /* Acetone correlates with BME688 #1 gas resistance change.
     * Higher acetone → lower resistance (MOX sensor behavior).
     * ppm = k / (R_gas / R_baseline) using calibration gain */
    float acetone_peak = features->sensors[0].peak;
    float acetone_plateau = features->sensors[0].plateau;

    /* Use calibration gain to convert to ppm */
    if (calib->span_gain[0] > 0) {
        float acetone_ppm = (acetone_peak - acetone_plateau) * calib->span_gain[0];
        if (acetone_ppm < 0) acetone_ppm = 0;
        result->acetone_ppm = (uint16_t)(acetone_ppm * 100);  /* Store ×100 */
    }

    /* --- H2 (from Membrapor H2-500 analog sensor) --- */
    /* Direct reading from ADC, already in ppm × 10 */
    float h2_max = 0;
    for (uint32_t i = 0; i < count; i++) {
        float h2 = (float)samples[i].h2_analog / 10.0f;
        if (h2 > h2_max) h2_max = h2;
    }
    result->h2_ppm = (uint16_t)(h2_max * 10);

    /* --- CH4 (from SenseAir S8 NDIR) --- */
    /* Direct reading in ppm */
    float ch4_max = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].s8_ch4 > ch4_max) ch4_max = samples[i].s8_ch4;
    }
    /* Subtract ambient CH4 baseline (~1.8 ppm) */
    if (ch4_max > 2.0f) {
        result->ch4_ppm = (uint16_t)((ch4_max - 1.8f) * 10);
    }

    /* --- Ethanol (from Spec DGS) --- */
    float ethanol_max = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].dgs_ethanol > ethanol_max) {
            ethanol_max = samples[i].dgs_ethanol;
        }
    }
    result->ethanol_ppm = (uint16_t)(ethanol_max * 100);  /* ppb → ppm × 100 */

    /* --- Isoprene (from BME688 #2) --- */
    float isoprene_peak = features->sensors[1].peak;
    if (calib->span_gain[1] > 0) {
        float isoprene_ppb = isoprene_peak * calib->span_gain[1];
        result->isoprene_ppb = (uint16_t)isoprene_ppb;
    }

    /* --- NH3 (from Spec DGS) --- */
    float nh3_max = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].dgs_nh3 > nh3_max) nh3_max = samples[i].dgs_nh3;
    }
    result->nh3_ppm = (uint16_t)(nh3_max * 10);

    /* --- H2S (from Spec DGS) --- */
    float h2s_max = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].dgs_h2s > h2s_max) h2s_max = samples[i].dgs_h2s;
    }
    result->h2s_ppb = (uint16_t)h2s_max;

    /* --- VOC Index (from BME688 #1 AUC) --- */
    /* Simplified IAQ: map AUC to 0-500 index */
    float voc_auc = features->sensors[0].auc;
    float voc_index = CLAMP(voc_auc / 100.0f, 0.0f, 500.0f);
    result->voc_index = (uint16_t)voc_index;

    /* --- CO2 Reference --- */
    uint16_t max_co2 = 0;
    for (uint32_t i = 0; i < count; i++) {
        if (samples[i].scd41_co2 > max_co2) max_co2 = samples[i].scd41_co2;
    }
    result->co2_ppm = max_co2;

    /* --- Environmental Data --- */
    result->temperature = (int16_t)(samples[count-1].temperature / 10);
    result->humidity = samples[count-1].humidity / 10;
    result->pressure = samples[count-1].pressure;
}

/* ========================================================================
 * Determine metabolic state from breath markers
 * ========================================================================
 */

uint8_t breath_classify_metabolic(const breath_result_t *result) {
    if (!result) return STATE_UNKNOWN;

    /* Extract normalized values */
    float acetone = (float)result->acetone_ppm / 100.0f;  /* ppm */
    float h2 = (float)result->h2_ppm / 10.0f;              /* ppm */
    float ch4 = (float)result->ch4_ppm / 10.0f;            /* ppm */

    /* Ketosis: acetone > 2.0 ppm is typically ketotic */
    if (acetone > 2.0f && h2 < 10.0f) {
        return STATE_KETOTIC;
    }

    /* Gut fermentation: elevated H2 or CH4 */
    if (h2 > 20.0f || ch4 > 10.0f) {
        return STATE_GUT_FERMENT;
    }

    /* Post-meal: moderate acetone, elevated H2 slightly */
    if (acetone > 0.5f && acetone < 2.0f && h2 > 5.0f) {
        return STATE_POSTMEAL;
    }

    /* Post-exercise: low acetone, low H2, possibly elevated isoprene */
    if (acetone < 0.3f && h2 < 3.0f) {
        return STATE_POSTEXERCISE;
    }

    /* Baseline: all markers low */
    if (acetone < 0.5f && h2 < 5.0f && ch4 < 3.0f) {
        return STATE_BASELINE;
    }

    return STATE_UNKNOWN;
}

/* ========================================================================
 * Get breath quality string
 * ========================================================================
 */

const char *breath_quality_name(uint8_t quality) {
    switch (quality) {
    case BREATH_VALID:       return "Valid";
    case BREATH_DEAD_SPACE:  return "Dead Space";
    case BREATH_CONTAMINATED: return "Contaminated";
    case BREATH_RETRY:       return "Retry";
    default:                 return "Unknown";
    }
}

/* ========================================================================
 * Get metabolic state string
 * ========================================================================
 */

const char *metabolic_state_name(uint8_t state) {
    switch (state) {
    case STATE_BASELINE:     return "Baseline";
    case STATE_KETOTIC:      return "Ketotic";
    case STATE_POSTMEAL:     return "Post-Meal";
    case STATE_POSTEXERCISE: return "Post-Exercise";
    case STATE_GUT_FERMENT:  return "Gut Fermentation";
    case STATE_UNKNOWN:      return "Unknown";
    default:                 return "Invalid";
    }
}

/* ========================================================================
 * Calculate breath confidence score
 * ========================================================================
 */

float breath_confidence(const breath_result_t *result,
                        const feature_vector_t *features) {
    if (!result || !features) return 0.0f;

    float confidence = 0.0f;

    /* CO2-based confidence (higher CO2 = more alveolar = more confident) */
    if (result->co2_ppm > 50000) {
        confidence += 0.3f;
    } else if (result->co2_ppm > CO2_ALVEOLAR_MIN) {
        confidence += 0.2f;
    } else {
        confidence += 0.05f;
    }

    /* Sensor response confidence (peak/plateau ratio) */
    float total_response = 0.0f;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (features->sensors[i].plateau > 0) {
            total_response += features->sensors[i].peak /
                             features->sensors[i].plateau;
        }
    }
    confidence += CLAMP(total_response / 20.0f, 0.0f, 0.3f);

    /* Feature consistency (rise time should be 0.5-5 seconds for valid breath) */
    float avg_rise = 0;
    int valid_rise = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (features->sensors[i].rise_time > 0.1f &&
            features->sensors[i].rise_time < 10.0f) {
            avg_rise += features->sensors[i].rise_time;
            valid_rise++;
        }
    }
    if (valid_rise > 0) {
        avg_rise /= valid_rise;
        if (avg_rise > 0.5f && avg_rise < 5.0f) {
            confidence += 0.2f;
        } else if (avg_rise > 0.2f && avg_rise < 8.0f) {
            confidence += 0.1f;
        }
    }

    /* Breath quality flag */
    if (result->breath_quality == BREATH_VALID) {
        confidence += 0.2f;
    }

    return CLAMP(confidence, 0.0f, 1.0f);
}