/*
 * logger.c — StudGuard local history ring buffer
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "logger.h"
#include <stdio.h>
#include <string.h>

void logger_init(log_state_t *state) {
    memset(state, 0, sizeof(*state));
}

void logger_append(log_state_t *state, uint32_t tick, const sg_device_status_t *status, const sg_measurement_t *measurement) {
    log_entry_t *entry = &state->entries[state->head];
    entry->tick = tick;
    entry->measurement = *measurement;
    entry->status = *status;
    state->head = (state->head + 1u) % LOG_CAPACITY;
    if (state->count < LOG_CAPACITY) {
        state->count++;
    }
}

void logger_export_latest(const log_state_t *state, char *buffer, size_t buffer_len, size_t max_entries) {
    size_t exported = 0u;
    size_t used = 0u;
    size_t i;
    if (buffer_len == 0u) {
        return;
    }

    used += (size_t)snprintf(buffer + used, buffer_len - used, "author=jayis1\n");
    for (i = 0u; i < state->count && exported < max_entries && used < buffer_len; ++i) {
        size_t idx = (state->head + LOG_CAPACITY - 1u - i) % LOG_CAPACITY;
        const log_entry_t *entry = &state->entries[idx];
        used += (size_t)snprintf(buffer + used, buffer_len - used,
                                 "tick=%u node=%u mode=%u leak=%.3f spread=%.3f conf=%.3f origin=%.2f temp=%.2f rh=%.2f\n",
                                 entry->tick,
                                 entry->status.node_id,
                                 (unsigned)entry->status.mode,
                                 entry->measurement.leak_activity,
                                 entry->measurement.wetness_spread,
                                 entry->measurement.confidence,
                                 entry->measurement.origin_band,
                                 entry->measurement.temperature_c,
                                 entry->measurement.humidity_rh);
        exported++;
    }
    if (used >= buffer_len) {
        buffer[buffer_len - 1u] = '\0';
    }
}
