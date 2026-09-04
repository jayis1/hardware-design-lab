/*
 * sash-sentinel/firmware/drivers/env.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "env.h"

#include <math.h>

static float pseudo_wave(uint32_t tick, float speed, float amplitude, float bias) {
    float x = (float)tick * speed;
    return sinf(x) * amplitude + cosf(x * 0.37f) * amplitude * 0.35f + bias;
}

void env_init(void) {
}

float env_compute_dew_point(float temp_c, float humidity_pct) {
    const float a = 17.27f;
    const float b = 237.7f;
    float bounded_h = board_clampf(humidity_pct, 1.0f, 100.0f);
    float gamma = ((a * temp_c) / (b + temp_c)) + logf(bounded_h / 100.0f);
    return (b * gamma) / (a - gamma);
}

env_snapshot_t env_sample(uint32_t tick, const device_config_t *config, float ventilation_bias) {
    env_snapshot_t snapshot;
    float diurnal = pseudo_wave(tick, 0.23f, 3.5f, 21.0f);
    float storm_push = pseudo_wave(tick + 11u, 0.13f, 1.2f, 0.0f);
    float moisture_surge = pseudo_wave(tick + 7u, 0.19f, 6.0f, 0.0f);

    snapshot.indoor_temp_c = diurnal + storm_push * 0.4f;
    snapshot.outdoor_est_temp_c = snapshot.indoor_temp_c - 7.5f - pseudo_wave(tick + 5u, 0.21f, 3.3f, 0.0f);
    snapshot.indoor_humidity_pct = board_clampf(45.0f + moisture_surge + ventilation_bias * 1.7f, 24.0f, 78.0f);
    snapshot.cavity_humidity_pct = board_clampf(snapshot.indoor_humidity_pct + 8.0f + pseudo_wave(tick + 17u, 0.31f, 8.5f, 0.0f) - ventilation_bias * 4.5f, 28.0f, 96.0f);
    snapshot.cavity_pressure_pa = config->expected_pressure_bias_pa + pseudo_wave(tick + 3u, 0.47f, 7.0f, 0.0f) - ventilation_bias * 10.0f;
    snapshot.sill_moisture_pct = board_clampf(12.0f + snapshot.cavity_humidity_pct * 0.32f + pseudo_wave(tick + 23u, 0.15f, 7.2f, 0.0f), 3.0f, 89.0f);
    snapshot.voc_index = board_clampf(14.0f + snapshot.sill_moisture_pct * 0.7f + pseudo_wave(tick + 29u, 0.11f, 10.0f, 0.0f), 0.0f, 100.0f);
    snapshot.dew_point_c = env_compute_dew_point(snapshot.indoor_temp_c, snapshot.cavity_humidity_pct);
    return snapshot;
}

float env_compute_mold_index(const sample_history_t *history) {
    if (history->count == 0u) {
        return 0.0f;
    }

    float humidity_acc = 0.0f;
    float moisture_acc = 0.0f;
    float cold_penalty = 0.0f;

    for (size_t i = 0; i < history->count; ++i) {
        const device_sample_t *sample = &history->samples[i];
        humidity_acc += sample->env.cavity_humidity_pct;
        moisture_acc += sample->env.sill_moisture_pct;
        if (sample->thermal.edge_cold_spot_c < sample->env.dew_point_c + 1.0f) {
            cold_penalty += 8.0f;
        }
    }

    float avg_humidity = humidity_acc / (float)history->count;
    float avg_moisture = moisture_acc / (float)history->count;
    float raw = (avg_humidity - 55.0f) * 1.2f + avg_moisture * 0.75f + cold_penalty / (float)history->count;
    return board_clampf(raw, 0.0f, 100.0f);
}
