/*
 * ble.c — StudGuard BLE telemetry encoder
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#include "ble.h"
#include "../registers.h"
#include <stdio.h>
#include <string.h>

void ble_init(void) {
    SG_BLE.CTRL = 1u;
    SG_BLE.TX_POWER = 4u;
}

void ble_encode_status(const sg_device_status_t *status, const sg_measurement_t *measurement, char *buffer, size_t buffer_len) {
    snprintf(buffer, buffer_len,
             "{\"author\":\"jayis1\",\"nodeId\":%u,\"mode\":%u,\"battery\":%.1f,\"intervalMs\":%u,"
             "\"tempC\":%.2f,\"rh\":%.2f,\"dewPointC\":%.2f,\"capMean\":%.3f,\"capDelta\":%.3f,"
             "\"acousticEnergy\":%.4f,\"decayMs\":%.2f,\"centroidHz\":%.1f,\"phaseStability\":%.3f,"
             "\"leakActivity\":%.3f,\"wetnessSpread\":%.3f,\"confidence\":%.3f,\"originBand\":%.2f,"
             "\"event\":%u,\"peers\":%u}",
             status->node_id,
             (unsigned)status->mode,
             status->battery_percent,
             status->interval_ms,
             measurement->temperature_c,
             measurement->humidity_rh,
             measurement->dew_point_c,
             measurement->cap_mean,
             measurement->cap_delta,
             measurement->acoustic_energy,
             measurement->acoustic_decay_ms,
             measurement->spectral_centroid_hz,
             measurement->phase_stability,
             measurement->leak_activity,
             measurement->wetness_spread,
             measurement->confidence,
             measurement->origin_band,
             (unsigned)measurement->event,
             (unsigned)measurement->peer_count);
    SG_BLE.STATUS = (uint32_t)strlen(buffer);
}

void ble_encode_peers(const sg_peer_snapshot_t *peers, size_t count, char *buffer, size_t buffer_len) {
    size_t i;
    size_t used = 0u;
    if (buffer_len == 0u) {
        return;
    }
    used += (size_t)snprintf(buffer + used, buffer_len - used, "{\"author\":\"jayis1\",\"peers\":[");
    for (i = 0; i < count && used < buffer_len; ++i) {
        used += (size_t)snprintf(buffer + used, buffer_len - used,
                                 "%s{\"id\":%u,\"z\":%.2f,\"leak\":%.3f,\"spread\":%.3f,\"conf\":%.3f}",
                                 i == 0u ? "" : ",",
                                 peers[i].id,
                                 peers[i].z,
                                 peers[i].leak_activity,
                                 peers[i].wetness_spread,
                                 peers[i].confidence);
    }
    if (used < buffer_len) {
        snprintf(buffer + used, buffer_len - used, "]}");
    } else {
        buffer[buffer_len - 1u] = '\0';
    }
}
