/*
 * model.c — Int8 model weights for the lichen-state classifier.
 *
 * These weights are a hand-constructed model that encodes the domain
 * knowledge that a lichen ecologist would use to interpret the sensor
 * readings. It is not a trained model from a dataset; it is a
 * knowledge-engineered starting point that can be refined with real
 * field data via walk-by BLE downloads and offline retraining.
 *
 * The architecture is 6 → 24 → 16 → 5 (HEALTHY, STRESSED, BLEACHING,
 * RECOVERING, DORMANT).
 *
 * Author: jayis1
 * License: MIT
 */

#include "model.h"

/* ------------------------------------------------------------------------ */
/* Layer 1 weights (24 × 6)                                                  */
/* Rows correspond to outputs; the 6 inputs are:
 *   [fv_fm, lndvi, chl_index, wetness, acoustic_events, uv_index]
 *
 * Hand-engineered heuristics:
 *   - High Fv/Fm + stable LNDVI → healthy.
 *   - Low Fv/Fm + high melanin (low chl_index) → stressed.
 *   - Very low Fv/Fm + falling LNDVI → bleaching.
 *   - Rising Fv/Fm + high wetness → recovering.
 *   - Frozen/dry (wetness ~0) + low Fv/Fm → dormant.
 */
const int8_t model_w1[24 * 6] = {
    /* neuron 0: healthy detector — strong positive on fv_fm */
     90,  40,  20,  10, -30, -10,
    /* neuron 1: stressed detector — negative on fv_fm, positive on uv */
    -60, -30, -20,   0,  20,  40,
    /* neuron 2: bleaching detector — very negative on fv_fm and lndvi */
    -90, -70, -40,   0,  30,  20,
    /* neuron 3: recovering detector — moderate positive fv_fm + wetness */
     50,  20,  10,  70, -10, -10,
    /* neuron 4: dormant detector — low wetness + low fv_fm */
    -40,   0,   0, -80,  10,   0,
    /* neurons 5..23: distributed feature detectors */
     30,  20,  10,  20, -10, -20,
    -20, -10, -20,  30,  10,  20,
     10,  30,  40, -10, -20,  10,
    -30, -20, -10,  40,  20,   0,
     50,  10, -20, -30,  10,  30,
     20,  40,  30, -10, -30,  20,
    -10, -30, -30,  50,  40, -10,
     60,  20,  10,   0, -20,  20,
    -40, -20,  10,  10,  30, -20,
     30, -10,  40,  20, -10,  10,
    -20,  30, -30,  60,  10,   0,
     10,  10,  10,  10,  10,  10,
    -10, -10, -10, -10, -10, -10,
     40,  20,   0,  20,   0,  20,
    -30, -20, -10,  10,  20,  30,
     20, -20,  30, -10,  10, -20,
     50,  30,  20,  10, -10, -30,
    -50, -30, -20,  20,  30,  10,
     15,  25, -15,  35,   5,  15,
};

const int32_t model_b1[24] = {
    -20, 10, 20, -10, 30,
     0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,
     0,  0,  0,  0,
};

const float model_scale1 = 0.0039f;  /* 1/256 */
const int8_t model_zp1 = 0;

/* ------------------------------------------------------------------------ */
/* Layer 2 weights (16 × 24)                                                 */
const int8_t model_w2[16 * 24] = {
    /* row 0: amplify healthy */
     90, -20, -30,  20, -40,  10,  10,  10, -10, -10,
     10,  20, -20,  30, -10,  10, -10,  20, -10,  10,
     30, -20,  10,  20,
    /* row 1: amplify stressed */
    -30,  80,  40, -10,  20, -10,  10, -10,  20,  10,
    -20, -10,  20, -10,  10, -10,  10, -20,  20, -10,
    -10,  30, -10, -20,
    /* row 2: amplify bleaching */
    -40,  30,  90, -20,  10, -20,  10, -10,  20, -10,
     10, -20,  20, -10,  10, -20,  10,  20, -10,  10,
    -20,  10,  20, -10,
    /* row 3: amplify recovering */
     20, -10, -20,  80, -30,  10, -10,  20, -10,  10,
    -20,  10,  20, -10,  10, -20,  10, -10,  20, -10,
     10, -20, -10,  20,
    /* row 4: amplify dormant */
    -30,  10,  10, -20,  90, -10,  20, -10,  10, -20,
     10, -10, -20,  10,  20, -10,  10, -20,  10,  20,
    -10,  10, -20, -10,
    /* rows 5..15: mixture detectors */
     10,  20, -10,  20, -10,  30, -20,  10, -10,  20,
     10, -10,  20, -10,  10, -20,  30,  10, -20,  10,
     20, -10,  10, -20,
    -20, -10,  20, -10,  10, -10,  40, -20,  10, -10,
    -20,  10, -10,  20, -10,  10, -20,  40, -10,  20,
    -10,  20, -10,  10,
     30,  10, -20,  10, -10,  20, -10,  10,  30, -20,
      10, -20,  10,  20, -10,  30, -10,  20, -10,  10,
     -20,  10,  20, -10,
    -10, -20,  10, -10,  20, -10, -20,  30,  10, -10,
      20,  10, -20, -10,  20, -10,  10, -20,  30, -10,
      10, -20,  10,  20,
     20,  10, -10,  20, -20, -10,  10, -20,  10,  30,
     -20,  10, -10,  20, -10,  10, -20,  10, -20,  30,
      10, -10,  20, -10,
    -10,  20,  10, -20,  10,  20, -10,  10, -20,  10,
      30, -10,  20, -10,  10, -20,  10, -20,  10,  30,
     -20,  10, -10,  20,
     10, -10,  20,  10, -20, -10,  20, -10,  10, -20,
      10,  30, -10,  20, -10,  10, -20,  10, -20,  10,
      30, -10,  20, -10,
    -20,  10, -10,  20,  10, -20,  10,  20, -10,  10,
     -20,  10,  30, -10,  20, -10,  10, -20,  10, -20,
      10,  30, -10,  20,
     30, -20,  10, -10,  20,  10, -20,  10,  20, -10,
      10, -20,  10,  30, -10,  20, -10,  10, -20,  10,
     -20,  10,  20, -10,
    -10,  20, -10,  10, -20,  10,  20, -10, -20,  10,
     -10,  20, -10,  10,  30, -20,  10, -10,  20, -10,
      10, -20,  30,  10,
     20, -10,  20, -10,  10, -20, -10,  20,  10, -20,
      20, -10,  10, -20,  10,  30, -10,  20, -10,  10,
     -20,  10, -10,  30,
};

const int32_t model_b2[16] = {
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
};

const float model_scale2 = 0.0039f;
const int8_t model_zp2 = 0;

/* ------------------------------------------------------------------------ */
/* Layer 3 weights (5 × 16) — output logits for the 5 classes                 */
const int8_t model_w3[5 * 16] = {
    /* HEALTHY */
     90, -30, -40,  20, -40,  10, -20,  10, -10,  20,
     -10, -10,  10,  20, -10,  20,
    /* STRESSED */
    -30,  80,  40, -10,  10, -10,  20, -20,  10, -10,
      20,  10, -10, -20,  10, -10,
    /* BLEACHING */
    -40,  40,  90, -20, -10, -20,  10, -10,  20,  10,
     -10, -20,  10,  10, -10, -20,
    /* RECOVERING */
     20, -10, -20,  90, -30,  10, -20,  10, -10,  20,
     -10,  20, -10,  10, -20,  10,
    /* DORMANT */
    -30,  10, -10, -20,  90, -10,  20, -10,  20, -10,
      10, -10, -20,  10,  20, -10,
};

const int32_t model_b3[5] = { 0, 0, 0, 0, 0 };

const float model_scale3 = 0.0039f;
const int8_t model_zp3 = 0;