/*
 * PipeWhisper BLE packet builder
 * Author: jayis1
 */
#include <stdio.h>
#include "ble.h"

size_t ble_build_status_packet(const pipe_snapshot_t *snapshot, char *buffer, size_t capacity)
{
    return (size_t)snprintf(buffer, capacity,
        "{\"device\":\"%s\",\"author\":\"%s\",\"minute\":%u,\"leak\":%.2f,\"freeze\":%.2f,\"hammer\":%.2f,\"health\":%.1f,\"battery\":%.1f}",
        PIPEWHISPER_DEVICE_NAME, PIPEWHISPER_AUTHOR, snapshot->minute_index,
        snapshot->inference.leak_confidence, snapshot->inference.freeze_risk,
        snapshot->pressure.hammer_score, snapshot->inference.health_index,
        snapshot->power.battery_percent);
}

size_t ble_build_report_packet(const pipe_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t capacity)
{
    return (size_t)snprintf(buffer, capacity,
        "REPORT|author=%s|minute=%u|reason=%s|events=%zu|fixture=%s|install=%.2f|priority=%.2f",
        PIPEWHISPER_AUTHOR, snapshot->minute_index,
        snapshot->inference.freeze_risk > snapshot->inference.leak_confidence ? "freeze" : "leak-or-hammer",
        events->count,
        snapshot->flow.fixture_similarity_washer > snapshot->flow.fixture_similarity_sink ? "washer" : "sink",
        snapshot->inference.install_quality, snapshot->inference.maintenance_priority);
}
