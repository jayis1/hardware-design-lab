/*
 * Canopy Sentinel display driver
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */
#include "display.h"

#include <stdio.h>

void display_init(void) {
    printf("[display] init complete\n");
}

void display_show_boot(const cs_device_state_t *device) {
    printf("[display] %s (%s) by %s\n", CS_PRODUCT_NAME, CS_FW_VERSION, CS_AUTHOR);
    printf("[display] serial=%s boot_count=%u active_crop=%s\n",
           device->serial, device->boot_count, cs_crop_name(device->active_crop));
}

void display_show_result(const cs_scan_result_t *result, const cs_power_state_t *power) {
    printf("[display] row=%s risk=%s score=%.1f dew_margin=%.2fC wet=%.2f spore=%.2f airflow=%.2f batt=%.1f%%\n",
           result->row_id,
           cs_risk_name(result->risk_level),
           result->risk_score,
           result->dew_margin_c,
           result->leaf.normalized_wetness,
           result->spore.fluorescence_index,
           result->airflow.stagnation_score,
           power->battery_percent);
}

void display_show_storage(const cs_log_t *log) {
    printf("[display] stored_scans=%zu ring_head=%zu\n", log->count, log->head);
}
