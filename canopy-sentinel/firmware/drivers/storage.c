/*
 * Canopy Sentinel storage driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "storage.h"

#include <stdio.h>
#include <string.h>

void storage_init(cs_log_t *log) {
    memset(log, 0, sizeof(*log));
}

void storage_append(cs_log_t *log, const cs_scan_result_t *result) {
    log->entries[log->head] = *result;
    log->head = (log->head + 1u) % CS_LOG_CAPACITY;
    if (log->count < CS_LOG_CAPACITY) {
        log->count++;
    }
}

void storage_export_csv(const cs_log_t *log, char *buffer, size_t buffer_size) {
    size_t used = 0;
    int n = snprintf(buffer, buffer_size,
                     "author,product,session,row,crop,risk,score,dew_margin,wetness,spore,stagnation\n");
    if (n < 0) {
        return;
    }
    used = (size_t)n;

    for (size_t i = 0; i < log->count && used < buffer_size; ++i) {
        const cs_scan_result_t *r = &log->entries[i];
        n = snprintf(buffer + used,
                     buffer_size - used,
                     "%s,%s,%u,%s,%s,%s,%.1f,%.2f,%.1f,%.1f,%.1f\n",
                     CS_AUTHOR,
                     CS_PRODUCT_NAME,
                     r->session_id,
                     r->row_id,
                     cs_crop_name(r->crop),
                     cs_risk_name(r->risk_level),
                     r->risk_score,
                     r->dew_margin_c,
                     r->leaf.normalized_wetness,
                     r->spore.fluorescence_index,
                     r->airflow.stagnation_score);
        if (n < 0) {
            return;
        }
        used += (size_t)n;
    }
}
