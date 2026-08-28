/*
 * VentLattice BLE formatter
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "ble.h"

size_t ble_build_status_packet(const vent_snapshot_t *snapshot, char *buffer, size_t buffer_size)
{
    int written = snprintf(
        buffer,
        buffer_size,
        "{\"device\":\"%s\",\"author\":\"%s\",\"hour\":%u,\"cfm\":%.1f,\"supplyC\":%.1f,\"roomC\":%.1f,\"service\":%.1f,\"stale\":%.2f,\"maint\":%.2f,\"reason\":\"%s\"}",
        VENTLATTICE_DEVICE_NAME,
        VENTLATTICE_AUTHOR,
        snapshot->hour_index,
        snapshot->airflow.airflow_cfm,
        snapshot->environment.supply_temp_c,
        snapshot->environment.room_temp_c,
        snapshot->inference.service_score,
        snapshot->inference.stale_air_risk,
        snapshot->inference.maintenance_priority,
        snapshot->inference.reason
    );
    if (written < 0) return 0u;
    if ((size_t)written >= buffer_size) return buffer_size - 1u;
    return (size_t)written;
}

size_t ble_build_report_packet(const vent_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t buffer_size)
{
    int written = snprintf(
        buffer,
        buffer_size,
        "{\"author\":\"%s\",\"events\":%zu,\"finalAlert\":%u,\"comfortWaste\":%.2f,\"condensationRisk\":%.2f,\"latestCode\":\"%s\"}",
        VENTLATTICE_AUTHOR,
        events->count,
        (unsigned)snapshot->inference.alert,
        snapshot->inference.comfort_waste,
        snapshot->inference.condensation_risk,
        events->count ? events->items[events->count - 1u].code : "NONE"
    );
    if (written < 0) return 0u;
    if ((size_t)written >= buffer_size) return buffer_size - 1u;
    return (size_t)written;
}
