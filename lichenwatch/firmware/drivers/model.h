/*
 * model.h — Int8 weights and quantization parameters for the lichen-state
 * classifier (6 → 24 → 16 → 5 MLP).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef MODEL_H
#define MODEL_H

#include <stdint.h>

/* Layer 1: 6 inputs → 24 outputs */
extern const int8_t  model_w1[24 * 6];
extern const int32_t model_b1[24];
extern const float   model_scale1;
extern const int8_t  model_zp1;

/* Layer 2: 24 inputs → 16 outputs */
extern const int8_t  model_w2[16 * 24];
extern const int32_t model_b2[16];
extern const float   model_scale2;
extern const int8_t  model_zp2;

/* Layer 3: 16 inputs → 5 outputs */
extern const int8_t  model_w3[5 * 16];
extern const int32_t model_b3[5];
extern const float   model_scale3;
extern const int8_t  model_zp3;

#endif /* MODEL_H */