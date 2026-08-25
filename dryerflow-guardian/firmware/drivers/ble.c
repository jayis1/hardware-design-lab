/*
 * ble.c — JSON telemetry packet encoding
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 */
#include <stdio.h>
#include "ble.h"

size_t ble_encode_packet(const char *device_id,
                         const dfg_sensor_frame_t *frame,
                         const dfg_health_metrics_t *metrics,
                         char *buffer,
                         size_t buffer_size) {
    if (!device_id || !frame || !metrics || !buffer || buffer_size == 0U) {
        return 0U;
    }

    return (size_t)snprintf(
        buffer,
        buffer_size,
        "{\"author\":\"jayis1\",\"device\":\"%s\",\"seq\":%u,"
        "\"run\":%u,\"pressure_pa\":%.2f,\"flow_cfm\":%.2f,"
        "\"temp_c\":%.2f,\"humidity_rh\":%.2f,\"co_ppm\":%.2f,"
        "\"vri\":%.2f,\"ces\":%.2f,\"bss\":%.2f,\"service_horizon\":%.2f,"
        "\"alerts\":%u}",
        device_id,
        frame->sequence,
        (unsigned)frame->run_state,
        frame->pressure_pa,
        frame->flow_cfm,
        frame->exhaust_temp_c,
        frame->humidity_rh,
        frame->co_ppm,
        metrics->vent_resistance_index,
        metrics->cycle_efficiency_score,
        metrics->backdraft_suspicion_score,
        metrics->service_horizon_loads,
        (unsigned)frame->alerts);
}
