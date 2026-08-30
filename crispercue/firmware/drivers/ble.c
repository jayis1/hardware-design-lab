/*
 * CrisperCue BLE packet builder
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include "ble.h"

size_t ble_build_status_packet(const crisper_snapshot_t *snapshot, char *buffer, size_t size)
{
    return (size_t)snprintf(buffer, size,
                            "{\"device\":\"%s\",\"author\":\"%s\",\"cycle\":%u,\"freshness\":%.1f,\"spoilage\":%.2f,\"ethylene\":%.3f,\"co2\":%.0f,\"mass_g\":%.1f,\"stage\":\"%s\"}",
                            CRISPERCUE_DEVICE_NAME,
                            CRISPERCUE_AUTHOR,
                            snapshot->cycle_index,
                            snapshot->inference.freshness_score,
                            snapshot->inference.spoilage_risk,
                            snapshot->gas.ethylene_ppm,
                            snapshot->gas.co2_ppm,
                            snapshot->mass.tray_mass_g,
                            snapshot->inference.stage);
}

size_t ble_build_report_packet(const crisper_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t size)
{
    const crisper_event_t *last = events->count ? &events->items[events->count - 1u] : NULL;
    return (size_t)snprintf(buffer, size,
                            "{\"report\":{\"cycle\":%u,\"alert\":%u,\"reason\":\"%s\",\"ventilation\":%.2f,\"value_usd\":%.2f,\"last_event\":\"%s\",\"last_code\":\"%s\"}}",
                            snapshot->cycle_index,
                            (unsigned)snapshot->inference.alert,
                            snapshot->inference.reason,
                            snapshot->inference.ventilation_demand,
                            snapshot->inference.shopper_value_left_usd,
                            last ? last->detail : "none",
                            last ? last->code : "NONE");
}
