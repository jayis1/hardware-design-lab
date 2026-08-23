/*
 * logger.h — StudGuard local history ring buffer
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_LOGGER_H
#define STUDGUARD_LOGGER_H

#include <stddef.h>
#include <stdint.h>
#include "../board.h"

typedef struct {
    uint32_t tick;
    sg_measurement_t measurement;
    sg_device_status_t status;
} log_entry_t;

typedef struct {
    log_entry_t entries[LOG_CAPACITY];
    size_t head;
    size_t count;
} log_state_t;

void logger_init(log_state_t *state);
void logger_append(log_state_t *state, uint32_t tick, const sg_device_status_t *status, const sg_measurement_t *measurement);
void logger_export_latest(const log_state_t *state, char *buffer, size_t buffer_len, size_t max_entries);

#endif
