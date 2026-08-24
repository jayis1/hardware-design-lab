/*
 * SplintSense BLE payload encoder
 * Author: jayis1
 */
#include <stdio.h>
#include "ble.h"

static const char *alert_name(alert_level_t level)
{
    switch (level) {
    case ALERT_NONE:
        return "none";
    case ALERT_INFO:
        return "info";
    case ALERT_CAUTION:
        return "caution";
    case ALERT_WARNING:
        return "warning";
    case ALERT_CRITICAL:
        return "critical";
    default:
        return "unknown";
    }
}

size_t ble_build_status_packet(const recovery_snapshot_t *snapshot, char *buffer, size_t capacity)
{
    if (capacity == 0u) {
        return 0u;
    }
    return (size_t)snprintf(
        buffer,
        capacity,
        "{\"device\":\"%s\",\"author\":\"%s\",\"fw\":\"%s\",\"minute\":%u,\"battery\":%.1f,\"rsi\":%.1f,\"fit\":%.1f,\"moisture\":%.1f,\"odor\":%.1f,\"impact\":%.2f,\"alert\":\"%s\"}",
        SPLINTSENSE_DEVICE_NAME,
        SPLINTSENSE_AUTHOR,
        SPLINTSENSE_FIRMWARE_VERSION,
        snapshot->minute_index,
        snapshot->power.battery_percent,
        snapshot->recovery_stability_index,
        snapshot->fit_score,
        snapshot->moisture.average,
        snapshot->odor_risk,
        snapshot->env.impact_g,
        alert_name(snapshot->alert)
    );
}

size_t ble_build_clinician_packet(const recovery_snapshot_t *snapshot, const alert_event_t *events, size_t event_count, char *buffer, size_t capacity)
{
    size_t used;
    size_t i;

    if (capacity == 0u) {
        return 0u;
    }

    used = (size_t)snprintf(
        buffer,
        capacity,
        "{\"author\":\"%s\",\"summary\":{\"minute\":%u,\"rsi\":%.1f,\"fit\":%.1f,\"odorRisk\":%.1f,\"compliance\":%.1f},\"events\":[",
        SPLINTSENSE_AUTHOR,
        snapshot->minute_index,
        snapshot->recovery_stability_index,
        snapshot->fit_score,
        snapshot->odor_risk,
        snapshot->compliance_score
    );

    for (i = 0u; i < event_count && used + 8u < capacity; ++i) {
        int written = snprintf(
            buffer + used,
            capacity - used,
            "%s{\"minute\":%u,\"level\":%u,\"code\":\"%s\"}",
            i == 0u ? "" : ",",
            events[i].minute_index,
            (unsigned)events[i].level,
            events[i].code
        );
        if (written < 0) {
            break;
        }
        used += (size_t)written;
    }

    if (used + 3u < capacity) {
        used += (size_t)snprintf(buffer + used, capacity - used, "]}");
    }
    return used;
}
