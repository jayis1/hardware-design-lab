/*
 * drivers/model.h — TinyML frost probability model header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_MODEL_H
#define FROSTSENTINEL_MODEL_H

#include <stdint.h>

/*
 * Run the int8 neural network forward pass on the given inputs.
 * Returns frost probability in Q8.8 fixed-point (0..256 = 0.0..1.0).
 */
int16_t model_infer(const int8_t inputs[8]);

/*
 * Convenience: compute frost probability from the current g_sys state.
 * Scales sensor readings to int8 model inputs and calls model_infer.
 */
int16_t model_compute_frost_probability(void);

#endif /* FROSTSENTINEL_MODEL_H */