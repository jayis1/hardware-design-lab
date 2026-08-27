/*
 * SealBeat event logger
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "logger.h"

void logger_init(event_log_t *events, snapshot_log_t *snapshots)
{
    events->count = 0u;
    snapshots->count = 0u;
}

void logger_push_event(event_log_t *events, uint32_t minute_index, alert_level_t level, const char *category, const char *detail)
{
    event_record_t *record;
    if (events->count >= SEALBEAT_MAX_EVENTS) return;
    record = &events->items[events->count++];
    record->minute_index = minute_index;
    record->level = level;
    snprintf(record->category, sizeof(record->category), "%s", category);
    snprintf(record->detail, sizeof(record->detail), "%s", detail);
}

void logger_push_snapshot(snapshot_log_t *snapshots, const appliance_snapshot_t *snapshot)
{
    if (snapshots->count >= SEALBEAT_MAX_SNAPSHOTS) return;
    snapshots->items[snapshots->count++] = *snapshot;
}

static void summarize_edges(const snapshot_log_t *snapshots, float *min_edge, float *max_tau)
{
    size_t i;
    *min_edge = 1.0f;
    *max_tau = 0.0f;
    for (i = 0u; i < snapshots->count; ++i) {
        const appliance_snapshot_t *s = &snapshots->items[i];
        if (s->seal.top_edge_score < *min_edge) *min_edge = s->seal.top_edge_score;
        if (s->seal.bottom_edge_score < *min_edge) *min_edge = s->seal.bottom_edge_score;
        if (s->thermal.recovery_tau_s > *max_tau) *max_tau = s->thermal.recovery_tau_s;
    }
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    size_t i;
    float min_edge;
    float max_tau;
    summarize_edges(snapshots, &min_edge, &max_tau);
    printf("\nEvent summary (%u events)\n", (unsigned)events->count);
    for (i = 0u; i < events->count; ++i) {
        printf("  [%02u] level=%s category=%s %s\n",
               (unsigned)events->items[i].minute_index,
               sb_alert_name(events->items[i].level),
               events->items[i].category,
               events->items[i].detail);
    }
    printf("\nTrend summary\n");
    printf("  snapshots=%u minEdgeScore=%.2f maxRecoveryTau=%.1fs\n",
           (unsigned)snapshots->count,
           min_edge,
           max_tau);
    if (snapshots->count > 0u) {
        const appliance_snapshot_t *last = &snapshots->items[snapshots->count - 1u];
        printf("  finalSeal=%.2f finalSafety=%.2f finalService=%.2f nights=%u cycles=%u\n",
               last->inference.seal_integrity,
               last->inference.safety_confidence,
               last->inference.service_score,
               last->door.night_cycles,
               last->door.cycle_count);
    }
}
