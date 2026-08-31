/*
 * Threshold Veil inference driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_INFERENCE_H
#define THRESHOLD_VEIL_INFERENCE_H

#include "board.h"

void inference_init(tv_inference_t *inf);
void inference_evaluate(tv_inference_t *inf,
                        const tv_env_frame_t *env,
                        const tv_acoustic_frame_t *ac,
                        const tv_seal_frame_t *seal,
                        tv_mode_t mode);
const char *inference_state_name(tv_state_t state);

#endif
