/*
 * classifier.h - Particle classifier header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef CLASSIFIER_H
#define CLASSIFIER_H
#include <stdint.h>
#include "board.h"

void classifier_init(void);
void classifier_set_aux_uv(int on);
int  classifier_predict(const roi_t *roi, float *logits);
const char *classifier_class_name(int idx);

#endif /* CLASSIFIER_H */