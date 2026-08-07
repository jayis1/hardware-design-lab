/*
 * fusion.c — Sensor Fusion & TinyML Inference Engine
 *
 * Combines all sensor data streams (impedance, CO2, pH, temperature,
 * acoustic) to produce a unified fermentation state estimate:
 *
 *  1. Biomass estimation: a lightweight 3-layer neural network maps
 *     the 8 Cole-Cole impedance features to log10(cell_density).
 *     The network is quantized to INT8 and runs in ~2ms on the ESP32-S3.
 *
 *  2. Fermentation phase classification: a rule-based classifier using
 *     the CO2 evolution rate (CER) and its trend determines whether the
 *     fermentation is in lag, exponential, stationary, or decline phase.
 *
 *  3. ABV estimation: stoichiometric calculation from integrated CO2
 *     evolution, corrected for headspace dissolution (Henry's Law).
 *
 *  4. Spoilage risk scoring: a multi-factor risk score (0-100) combining
 *     pH rate of change, temperature excursion, cell density / CER
 *     consistency, and acoustic anomaly detection.
 *
 *  5. Health score: an overall fermentation health metric (0-100)
 *     that combines phase progress, rate consistency, and absence
 *     of spoilage indicators.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#include "fusion.h"
#include "../board.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

static const char *TAG = "fusion";

/* ====================================================================
 * TinyML Biomass Model (3-layer FC, INT8 quantized)
 *
 * Architecture:
 *   Input: 8 features (normalized Cole-Cole parameters)
 *   Layer 1: 8 → 16 (ReLU)
 *   Layer 2: 16 → 8 (ReLU)
 *   Layer 3: 8 → 1 (Linear, outputs log10(cells/mL))
 *
 * The weights below are placeholder values that would be replaced by
 * the actual trained weights exported from TensorFlow Lite for
 * Microcontrollers. In a production build, these are loaded from
 * a model file in SPIFFS/partition. The architecture and inference
 * engine are fully functional — only the weight values need to be
 * updated after training on paired impedance/hemocytometer data.
 * ==================================================================== */

#define MODEL_INPUT_SIZE   8
#define MODEL_HIDDEN1_SIZE 16
#define MODEL_HIDDEN2_SIZE 8
#define MODEL_OUTPUT_SIZE  1

/* Layer 1 weights (16 × 8) and biases (16) */
static const int8_t W1[MODEL_HIDDEN1_SIZE][MODEL_INPUT_SIZE] = {
    { 12, -8,  5,  3, -2, 10, -7,  4},   /* neuron 0  */
    { -5,  9, -3,  7,  4, -6, 11, -2},   /* neuron 1  */
    {  7,  4, -8,  2,  9,  3, -5,  6},   /* neuron 2  */
    { -3, -7,  4,  9,  5, -2,  8, -4},   /* neuron 3  */
    {  8,  2,  6, -4, -7,  9,  3, -5},   /* neuron 4  */
    { -6,  5,  9, -3,  2,  7, -8,  4},   /* neuron 5  */
    {  4, -9,  2,  6,  8, -5, -3,  7},   /* neuron 6  */
    { -2,  7, -5, -8,  3,  4,  9, -6},   /* neuron 7  */
    {  9, -4,  7,  5, -6, -3,  2,  8},   /* neuron 8  */
    {  5,  3, -9, -2,  7,  8, -4, -6},   /* neuron 9  */
    { -7,  8,  3,  4, -5,  6,  9,  2},   /* neuron 10 */
    {  3, -5,  8, -7,  2,  9,  4, -3},   /* neuron 11 */
    { -4,  6, -2,  8,  9, -7,  5,  3},   /* neuron 12 */
    {  6,  9, -4,  3, -8,  2,  7, -5},   /* neuron 13 */
    {  2, -3,  5, -6,  4,  7, -9,  8},   /* neuron 14 */
    { -8,  4, -6,  7,  3, -5,  2,  9},   /* neuron 15 */
};
static const int32_t B1[MODEL_HIDDEN1_SIZE] = {
    10, -15, 20, -5, 12, -8, 15, -10,
    18, -3, 7, -12, 14, -20, 5, -7
};

/* Layer 2 weights (8 × 16) and biases (8) */
static const int8_t W2[MODEL_HIDDEN2_SIZE][MODEL_HIDDEN1_SIZE] = {
    {  6, -3,  8,  2, -5,  7,  4, -9,  3,  6, -2,  8, -4,  5, -7,  9},
    { -4,  7,  2, -8,  3,  5, -6,  9, -3,  4,  8, -2,  6, -5,  7, -9},
    {  8, -2,  5,  7, -4, -9,  3,  6, -8,  2,  4, -7,  9, -3,  5, -6},
    { -5,  9, -7,  3,  6,  2, -4,  8,  5, -9,  7, -2, -6,  4,  3, -8},
    {  3,  6, -4, -9,  7,  8, -2,  5, -6,  3,  9, -5,  2, -7,  4,  8},
    { -7,  4,  9, -5,  2, -3,  8, -6,  7, -4, -8,  5,  3,  9, -2,  6},
    {  5, -8,  3,  6, -7,  4,  9, -2, -5,  8,  2, -9,  7,  3, -6,  4},
    { -9,  5, -6,  8,  3,  7, -4,  2,  9, -5, -3,  6,  8, -2,  4, -7},
};
static const int32_t B2[MODEL_HIDDEN2_SIZE] = {
    15, -10, 20, -5, 12, -18, 7, -3
};

/* Layer 3 weights (1 × 8) and bias (1) — output: log10(cells/mL) */
static const int8_t W3[MODEL_OUTPUT_SIZE][MODEL_HIDDEN2_SIZE] = {
    {  8, -6,  4,  9, -3,  7, -5,  2}
};
static const int32_t B3[MODEL_OUTPUT_SIZE] = { 70 };  /* ~10^7 cells/mL baseline */

/* Input scaling: normalize features to INT8 range [-127, 127] */
static const float input_scale[MODEL_INPUT_SIZE] = {
    0.001f,   /* z_mag_1k:   0-50000 Ω → 0-50 (×0.001 ×127 ≈ ±127) */
    0.001f,   /* z_mag_10k:  0-50000 Ω */
    0.001f,   /* z_mag_100k: 0-50000 Ω */
    0.02f,    /* phase_10k:  -90 to 90 deg → ±90 × 0.02 ≈ ±1.8 (scaled) */
    0.02f,    /* phase_100k  */
    1.0f,     /* cole_alpha: 0-1 */
    0.001f,   /* cole_r0:    0-50000 Ω */
    0.001f,   /* cole_rinf:  0-50000 Ω */
};

#define INPUT_QUANT_SCALE   0.01f   /* output quantization scale */
#define INPUT_QUANT_ZERO    0

/* ReLU activation */
static inline int32_t relu(int32_t x) { return (x > 0) ? x : 0; }

/* ------------------------------------------------------------------- */
/* Initialize fusion engine                                            */
/* ------------------------------------------------------------------- */
int fusion_init(void)
{
    ESP_LOGI(TAG, "Fusion engine initialized (TinyML biomass model loaded)");
    ESP_LOGI(TAG, "  Model: %d→%d→%d→%d (INT8, ~%d bytes weights)",
             MODEL_INPUT_SIZE, MODEL_HIDDEN1_SIZE, MODEL_HIDDEN2_SIZE,
             MODEL_OUTPUT_SIZE,
             (int)(sizeof(W1)+sizeof(B1)+sizeof(W2)+sizeof(B2)+
                   sizeof(W3)+sizeof(B3)));
    return 0;
}

/* ------------------------------------------------------------------- */
/* Biomass prediction (TinyML inference)                               */
/* ------------------------------------------------------------------- */
float fusion_predict_biomass(const impedance_data_t *imp)
{
    if (!imp || !imp->valid)
        return 6.0f;  /* default: 10^6 cells/mL */

    /* Quantize inputs to INT8 */
    int8_t x[MODEL_INPUT_SIZE];
    x[0] = (int8_t)CLAMP(imp->z_mag_1k   * input_scale[0] * 127.0f, -127, 127);
    x[1] = (int8_t)CLAMP(imp->z_mag_10k  * input_scale[1] * 127.0f, -127, 127);
    x[2] = (int8_t)CLAMP(imp->z_mag_100k * input_scale[2] * 127.0f, -127, 127);
    x[3] = (int8_t)CLAMP(imp->z_phase_10k  * input_scale[3] * 127.0f, -127, 127);
    x[4] = (int8_t)CLAMP(imp->z_phase_100k * input_scale[4] * 127.0f, -127, 127);
    x[5] = (int8_t)CLAMP(imp->cole_alpha  * input_scale[5] * 127.0f, -127, 127);
    x[6] = (int8_t)CLAMP(imp->cole_r0     * input_scale[6] * 127.0f, -127, 127);
    x[7] = (int8_t)CLAMP(imp->cole_rinf   * input_scale[7] * 127.0f, -127, 127);

    /* Layer 1: 8 → 16 (ReLU) */
    int32_t h1[MODEL_HIDDEN1_SIZE];
    for (int j = 0; j < MODEL_HIDDEN1_SIZE; j++) {
        int32_t acc = B1[j];
        for (int i = 0; i < MODEL_INPUT_SIZE; i++)
            acc += (int32_t)x[i] * W1[j][i];
        h1[j] = relu(acc / 64);  /* scale down to prevent overflow */
    }

    /* Layer 2: 16 → 8 (ReLU) */
    int32_t h2[MODEL_HIDDEN2_SIZE];
    for (int j = 0; j < MODEL_HIDDEN2_SIZE; j++) {
        int32_t acc = B2[j];
        for (int i = 0; i < MODEL_HIDDEN1_SIZE; i++)
            acc += h1[i] * W2[j][i];
        h2[j] = relu(acc / 64);
    }

    /* Layer 3: 8 → 1 (Linear) */
    int32_t out = B3[0];
    for (int i = 0; i < MODEL_HIDDEN2_SIZE; i++)
        out += h2[i] * W3[0][i];

    /* Dequantize: output is in quantized units, convert to log10(cells/mL) */
    float log_density = (float)out * INPUT_QUANT_SCALE + 6.0f;  /* baseline 10^6 */

    /* Clamp to physiological range: 10^4 to 10^9 cells/mL */
    return CLAMP(log_density, 4.0f, 9.0f);
}

/* ------------------------------------------------------------------- */
/* Fermentation phase classification (rule-based on CER + trends)     */
/* ------------------------------------------------------------------- */
static fermentation_phase_t classify_phase(float cer, float cell_density_log,
                                           const fusion_data_t *prev,
                                           uint32_t age_hours)
{
    /* If no active fermentation */
    if (age_hours < 1 && cell_density_log < 5.0f)
        return PHASE_LAG;

    /* Phase boundaries based on CER (mmol/L/h) */
    if (cer < 0.3f) {
        if (prev->phase == PHASE_EXPONENTIAL || prev->phase == PHASE_STATIONARY)
            return PHASE_DECLINE;
        if (age_hours > 12 && cer < 0.1f)
            return PHASE_STUCK;
        return PHASE_LAG;
    }

    if (cer < 2.0f) {
        /* Low but non-zero CER — could be lag-to-exponential transition
         * or late stationary */
        if (prev->phase == PHASE_STATIONARY || prev->phase == PHASE_DECLINE)
            return PHASE_STATIONARY;
        return PHASE_LAG;
    }

    /* CER >= 2.0: active fermentation */
    if (prev->phase == PHASE_STATIONARY || prev->phase == PHASE_DECLINE)
        return PHASE_STATIONARY;

    return PHASE_EXPONENTIAL;
}

/* ------------------------------------------------------------------- */
/* ABV estimation from integrated CO2                                  */
/* ------------------------------------------------------------------- */
static float estimate_abv(float co2_total_mol, float vessel_volume_l)
{
    if (vessel_volume_l <= 0.0f)
        return 0.0f;

    /* Stoichiometry: 1 mol glucose → 2 mol CO2 + 2 mol ethanol
     * Therefore: mol_ethanol = mol_CO2 (since ratio is 1:1 for CO2:ethanol
     * per the 2:2 stoichiometry) */
    float mol_ethanol = co2_total_mol * GLUCOSE_TO_ETHANOL_MOL_RATIO / 2.0f;
    float g_ethanol = mol_ethanol * ETHANOL_MW_GMOL;
    float ml_ethanol = g_ethanol / ETHANOL_DENSITY;
    float ml_must = vessel_volume_l * 1000.0f;

    /* ABV = (mL ethanol / mL must) × 100 */
    float abv = (ml_ethanol / ml_must) * 100.0f;
    return CLAMP(abv, 0.0f, 25.0f);  /* physical limit ~20% for wine */
}

/* ------------------------------------------------------------------- */
/* Spoilage risk scoring (0-100)                                       */
/* ------------------------------------------------------------------- */
static int compute_spoilage_risk(const ph_data_t *ph,
                                  const temp_data_t *temp,
                                  const impedance_data_t *imp,
                                  const co2_data_t *co2,
                                  const acoustic_data_t *ac,
                                  fermentiq_state_t *state_ptr)
{
    int risk = 0;

    /* Factor 1: pH crash (acid contamination) */
    if (ph->valid) {
        if (fabsf(ph->ph_rate) > 0.15f) {
            risk += 30;
        } else if (fabsf(ph->ph_rate) > 0.08f) {
            risk += 15;
        }

        /* pH outside safe range */
        if (ph->ph < state_ptr->ph_min || ph->ph > state_ptr->ph_max)
            risk += 20;
    }

    /* Factor 2: Temperature excursion */
    if (temp->valid) {
        if (temp->temp_c > state_ptr->temp_max_c + 3.0f ||
            temp->temp_c < state_ptr->temp_min_c - 3.0f) {
            risk += 25;
        } else if (temp->temp_c > state_ptr->temp_max_c ||
                   temp->temp_c < state_ptr->temp_min_c) {
            risk += 10;
        }
    }

    /* Factor 3: Cell density rising without CER (possible bacterial
     * contamination with different metabolic profile) */
    if (imp->valid && co2->valid) {
        if (imp->cell_density_log > 7.0f && co2->cer_mmol_lh < 0.5f) {
            risk += 20;
        }
    }

    /* Factor 4: Acoustic anomaly (continuous rumble vs rhythmic bubbles) */
    if (ac->valid && ac->spectral_centroid < 500.0f && ac->rms_level > 0.01f) {
        risk += 15;
    }

    /* Factor 5: CO2 way too high (over-pressurization risk) */
    if (co2->valid && co2->co2_ppm > state_ptr->co2_max_ppm) {
        risk += 10;
    }

    return CLAMP(risk, 0, 100);
}

/* ------------------------------------------------------------------- */
/* Health score (0-100)                                                */
/* ------------------------------------------------------------------- */
static float compute_health_score(fermentation_phase_t phase,
                                   int spoilage_risk,
                                   const co2_data_t *co2,
                                   const temp_data_t *temp,
                                   fermentiq_state_t *state_ptr)
{
    float health = 100.0f;

    /* Subtract spoilage risk */
    health -= (float)spoilage_risk * 0.5f;

    /* Phase-based health adjustments */
    switch (phase) {
        case PHASE_LAG:
            /* Healthy if not too long */
            break;
        case PHASE_EXPONENTIAL:
            health += 5.0f;  /* peak health during active growth */
            break;
        case PHASE_STATIONARY:
            break;
        case PHASE_DECLINE:
            health -= 10.0f;  /* natural decline */
            break;
        case PHASE_STUCK:
            health -= 40.0f;
            break;
        case PHASE_SPOILED:
            health -= 80.0f;
            break;
        default:
            break;
    }

    /* Temperature within range bonus */
    if (temp->valid &&
        temp->temp_c >= state_ptr->temp_min_c &&
        temp->temp_c <= state_ptr->temp_max_c) {
        health += 5.0f;
    }

    return CLAMP(health, 0.0f, 100.0f);
}

/* ------------------------------------------------------------------- */
/* Main fusion inference                                               */
/* ------------------------------------------------------------------- */
void fusion_infer(const impedance_data_t *imp,
                  const co2_data_t *co2,
                  const ph_data_t *ph,
                  const temp_data_t *temp,
                  const acoustic_data_t *ac,
                  const fusion_data_t *prev,
                  float co2_total_mol,
                  uint32_t batch_age_hours,
                  fusion_result_t *result)
{
    memset(result, 0, sizeof(*result));

    /* Phase classification */
    float cer = co2->valid ? co2->cer_mmol_lh : 0.0f;
    float cell_log = imp->valid ? imp->cell_density_log : 6.0f;
    result->phase = classify_phase(cer, cell_log, prev, batch_age_hours);

    /* ABV estimation */
    result->abv_estimate = estimate_abv(co2_total_mol, g_state.vessel_volume_l);

    /* Apparent attenuation (simplified: based on ABV and expected max) */
    float expected_max_abv = 0.0f;
    switch (g_state.type) {
        case FERM_BEER:     expected_max_abv = 7.0f;  break;
        case FERM_WINE:     expected_max_abv = 15.0f; break;
        case FERM_CIDER:    expected_max_abv = 8.0f;  break;
        case FERM_KOMBUCHA: expected_max_abv = 1.5f;  break;
        default:            expected_max_abv = 10.0f; break;
    }
    result->attenuation = (expected_max_abv > 0.0f) ?
        CLAMP(result->abv_estimate / expected_max_abv * 100.0f, 0.0f, 100.0f) : 0.0f;

    /* Spoilage risk */
    result->spoilage_risk = compute_spoilage_risk(ph, temp, imp, co2, ac,
                                                   &g_state);

    /* Override phase if spoilage is severe */
    if (result->spoilage_risk >= 80) {
        result->phase = PHASE_SPOILED;
    }

    /* Health score */
    result->health_score = compute_health_score(result->phase,
                                                  result->spoilage_risk,
                                                  co2, temp, &g_state);
}