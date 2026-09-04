/*
 * sash-sentinel/firmware/drivers/latch.c
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */

#include "latch.h"

#include <math.h>

void latch_init(void) {
}

latch_snapshot_t latch_sample(uint32_t tick, const thermal_snapshot_t *thermal) {
    latch_snapshot_t snapshot;
    float cold_factor = board_clampf((9.0f - thermal->edge_cold_spot_c) * 0.35f, 0.0f, 3.0f);

    snapshot.latch_force_n = 18.0f + cosf((float)tick * 0.13f) * 2.5f - cold_factor;
    snapshot.sash_offset_mm = board_clampf(0.4f + sinf((float)tick * 0.09f) * 0.6f + cold_factor * 0.25f, 0.0f, 4.5f);
    snapshot.vibration_rms = board_clampf(0.05f + fabsf(sinf((float)tick * 0.33f)) * 0.12f + snapshot.sash_offset_mm * 0.05f, 0.0f, 1.0f);
    snapshot.travel_cycles = (float)(tick % 730u) * 0.25f;
    snapshot.latch_closed = snapshot.sash_offset_mm < 1.7f;
    snapshot.tamper_event = (tick % 17u) == 0u && snapshot.vibration_rms > 0.14f;
    return snapshot;
}

float latch_health_score(const sample_history_t *history) {
    if (history->count == 0u) {
        return 100.0f;
    }

    float penalty = 0.0f;
    for (size_t i = 0; i < history->count; ++i) {
        const latch_snapshot_t *l = &history->samples[i].latch;
        if (!l->latch_closed) {
            penalty += 7.0f;
        }
        penalty += l->sash_offset_mm * 3.8f;
        penalty += l->vibration_rms * 22.0f;
        if (l->tamper_event) {
            penalty += 4.0f;
        }
    }

    float score = 100.0f - penalty / (float)history->count;
    return board_clampf(score, 0.0f, 100.0f);
}
