/*
 * sash-sentinel/firmware/drivers/airflow.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "airflow.h"

#include <math.h>

void airflow_init(void) {
}

airflow_snapshot_t airflow_sample(uint32_t tick, const env_snapshot_t *env, const latch_snapshot_t *latch) {
    airflow_snapshot_t snapshot;
    float pressure_drive = fabsf(env->cavity_pressure_pa) * 0.012f;
    float latch_leak = latch->sash_offset_mm * 0.19f;
    float gust = fabsf(sinf((float)tick * 0.41f)) * 0.22f;

    snapshot.leak_velocity_mps = board_clampf(pressure_drive + latch_leak + gust, 0.0f, 2.6f);
    snapshot.acoustic_leak_score = board_clampf(snapshot.leak_velocity_mps * 28.0f + fabsf(cosf((float)tick * 0.27f)) * 16.0f, 0.0f, 100.0f);
    snapshot.pressure_pulse_pa = env->cavity_pressure_pa * (0.4f + latch->sash_offset_mm * 0.1f);
    snapshot.gust_event = snapshot.leak_velocity_mps > 1.1f;
    snapshot.rain_pattern = env->cavity_humidity_pct > 73.0f && fabsf(snapshot.pressure_pulse_pa) > 2.6f;
    return snapshot;
}

float airflow_energy_loss_score(const sample_history_t *history) {
    if (history->count == 0u) {
        return 0.0f;
    }

    float leak_total = 0.0f;
    float pressure_total = 0.0f;
    for (size_t i = 0; i < history->count; ++i) {
        leak_total += history->samples[i].airflow.leak_velocity_mps;
        pressure_total += fabsf(history->samples[i].airflow.pressure_pulse_pa);
    }

    float score = leak_total * 20.0f / (float)history->count + pressure_total * 2.2f / (float)history->count;
    return board_clampf(score, 0.0f, 100.0f);
}
