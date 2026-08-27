/*
 * SealBeat BLE packet builder
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include "ble.h"
#include "inference.h"

size_t ble_build_status_packet(const appliance_snapshot_t *snapshot, char *buffer, size_t capacity)
{
    return (size_t)snprintf(buffer, capacity,
                            "{\"device\":\"%s\",\"author\":\"%s\",\"minute\":%u,\"profile\":\"sim\",\"seal\":%.2f,\"safety\":%.2f,\"hinge\":%.2f,\"edge\":\"%s\",\"battery\":%.1f,\"alert\":\"%s\"}",
                            SEALBEAT_DEVICE_NAME,
                            SEALBEAT_AUTHOR,
                            snapshot->minute_index,
                            snapshot->inference.seal_integrity,
                            snapshot->inference.safety_confidence,
                            snapshot->inference.hinge_wear,
                            snapshot->seal.top_edge_score < snapshot->seal.bottom_edge_score ? "top" : "bottom",
                            snapshot->power.battery_percent,
                            sb_alert_name(snapshot->inference.alert));
}

size_t ble_build_report_packet(const appliance_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t capacity)
{
    return (size_t)snprintf(buffer, capacity,
                            "{\"device\":\"%s\",\"events\":%u,\"reason\":\"%s\",\"closure\":%.2f,\"energy\":%.2f,\"warmRebound\":%.2f,\"recoveryTau\":%.1f,\"maintenance\":%.2f}",
                            SEALBEAT_DEVICE_NAME,
                            (unsigned)events->count,
                            inference_primary_reason(snapshot),
                            snapshot->inference.closure_quality,
                            snapshot->inference.energy_penalty,
                            snapshot->thermal.warm_rebound_c,
                            snapshot->thermal.recovery_tau_s,
                            snapshot->inference.maintenance_priority);
}
