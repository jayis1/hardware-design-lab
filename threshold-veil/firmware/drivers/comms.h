/*
 * Threshold Veil communications driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_COMMS_H
#define THRESHOLD_VEIL_COMMS_H

#include <stddef.h>

#include "board.h"

void comms_format_frame(char *buffer,
                        size_t length,
                        const tv_env_frame_t *env,
                        const tv_acoustic_frame_t *ac,
                        const tv_seal_frame_t *seal,
                        const tv_power_frame_t *power,
                        const tv_inference_t *inf,
                        tv_mode_t mode);

#endif
