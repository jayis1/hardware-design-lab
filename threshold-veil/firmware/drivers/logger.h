/*
 * Threshold Veil logger driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#ifndef THRESHOLD_VEIL_LOGGER_H
#define THRESHOLD_VEIL_LOGGER_H

#include <stddef.h>

#include "board.h"

typedef struct {
    tv_log_entry_t entries[TV_EVENT_LOG_SIZE];
    size_t head;
    size_t count;
} tv_logger_t;

void logger_init(tv_logger_t *logger);
void logger_push(tv_logger_t *logger,
                 uint32_t tick,
                 const tv_inference_t *inf,
                 const tv_env_frame_t *env,
                 const tv_power_frame_t *power);
void logger_print(const tv_logger_t *logger);

#endif
