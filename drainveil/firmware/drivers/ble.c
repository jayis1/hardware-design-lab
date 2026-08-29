/*
 * DrainVeil BLE packet builders
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include "ble.h"

size_t ble_build_status_packet(const drain_snapshot_t *snapshot, char *buffer, size_t buffer_size)
{
    int written = snprintf(buffer, buffer_size,
                           "{\"device\":\"%s\",\"author\":\"%s\",\"fw\":\"%s\",\"minute\":%u,\"profile\":\"%s\",\"flow_lpm\":%.2f,\"fill_pct\":%.2f,\"clog_risk\":%.2f,\"odor_risk\":%.2f,\"freeze_risk\":%.2f,\"alert\":\"%s\"}",
                           DRAINVEIL_DEVICE_NAME,
                           DRAINVEIL_AUTHOR,
                           DRAINVEIL_FIRMWARE_VERSION,
                           snapshot->minute_index,
                           dv_profile_name(INSTALL_PROFILE_GREASE_INTERCEPTOR),
                           snapshot->flow.flow_lpm,
                           snapshot->flow.fill_height_percent,
                           snapshot->inference.clog_risk,
                           snapshot->inference.odor_risk,
                           snapshot->inference.freeze_risk,
                           dv_alert_name(snapshot->inference.alert));
    if (written < 0) return 0u;
    return (size_t)written;
}

size_t ble_build_report_packet(const drain_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t buffer_size)
{
    size_t used = 0u;
    int written = snprintf(buffer, buffer_size,
                           "REPORT|device=%s|author=%s|reason=%s|priority=%.2f|events=%zu",
                           DRAINVEIL_DEVICE_NAME,
                           DRAINVEIL_AUTHOR,
                           snapshot->inference.reason,
                           snapshot->inference.maintenance_priority,
                           events->count);
    if (written < 0) return 0u;
    used = (size_t)written;
    for (size_t i = 0u; i < events->count && used + 32u < buffer_size; ++i) {
        written = snprintf(buffer + used, buffer_size - used, "\n%02zu:%u:%s:%s",
                           i,
                           events->items[i].minute_index,
                           events->items[i].code,
                           events->items[i].detail);
        if (written < 0) break;
        used += (size_t)written;
    }
    return used;
}
