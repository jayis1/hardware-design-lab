/*
 * model_weights.h - Quantized model weights header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef MODEL_WEIGHTS_H
#define MODEL_WEIGHTS_H
#include <stdint.h>

extern const int8_t  g_conv0_w[432];
extern const int8_t  g_conv1_w[4608];
extern const int8_t  g_conv2_w[18432];
extern const int8_t  g_fc_w[2560];

extern const int32_t g_conv0_b[16];
extern const int32_t g_conv1_b[32];
extern const int32_t g_conv2_b[64];
extern const int32_t g_fc_b[40];

extern const char *g_class_names[40];

void model_weights_ensure(void);

#endif /* MODEL_WEIGHTS_H */