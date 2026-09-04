/*
 * sash-sentinel/firmware/drivers/thermal.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "thermal.h"

#include <math.h>

void thermal_init(void) {
}

thermal_snapshot_t thermal_sample(uint32_t tick, const env_snapshot_t *env) {
    thermal_snapshot_t snapshot;
    float mean_frame = env->indoor_temp_c - 3.0f + sinf((float)tick * 0.17f) * 0.9f;
    float exterior_pull = (env->indoor_temp_c - env->outdoor_est_temp_c) * 0.18f;

    for (size_t i = 0; i < FW_WINDOW_ZONES; ++i) {
        float zone_bias = (float)i * 0.4f;
        snapshot.frame_temp_c[i] = mean_frame - zone_bias - exterior_pull * (0.4f + (float)i * 0.12f);
        snapshot.glass_temp_c[i] = mean_frame - 1.3f - zone_bias * 0.7f - exterior_pull * (0.7f + (float)i * 0.15f);
        snapshot.seal_temp_c[i] = mean_frame - 1.8f - zone_bias * 0.35f - exterior_pull * (0.9f + (float)i * 0.10f);
    }

    float frame_avg = thermal_mean_frame_temp(&snapshot);
    float seal_min = board_min(snapshot.seal_temp_c, FW_WINDOW_ZONES);
    snapshot.thermal_gradient_c = frame_avg - board_average(snapshot.glass_temp_c, FW_WINDOW_ZONES);
    snapshot.edge_cold_spot_c = seal_min;
    snapshot.frost_signature = (snapshot.edge_cold_spot_c < 1.8f) && (env->cavity_humidity_pct > 70.0f);
    return snapshot;
}

float thermal_mean_frame_temp(const thermal_snapshot_t *snapshot) {
    return board_average(snapshot->frame_temp_c, FW_WINDOW_ZONES);
}
