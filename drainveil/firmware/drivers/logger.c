/*
 * DrainVeil logger helpers
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "logger.h"

void logger_init(event_log_t *events, snapshot_log_t *snapshots)
{
    memset(events, 0, sizeof(*events));
    memset(snapshots, 0, sizeof(*snapshots));
}

void logger_push_event(event_log_t *events, uint32_t minute_index, alert_level_t level, const char *code, const char *detail)
{
    drain_event_t *event;
    if (events->count >= DRAINVEIL_EVENT_CAPACITY) return;
    event = &events->items[events->count++];
    event->minute_index = minute_index;
    event->level = level;
    snprintf(event->code, sizeof(event->code), "%s", code);
    snprintf(event->detail, sizeof(event->detail), "%s", detail);
}

void logger_push_snapshot(snapshot_log_t *snapshots, const drain_snapshot_t *snapshot)
{
    if (snapshots->count >= DRAINVEIL_SNAPSHOT_CAPACITY) return;
    snapshots->items[snapshots->count++] = *snapshot;
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    float clog_total = 0.0f;
    float odor_total = 0.0f;
    float freeze_total = 0.0f;
    size_t i;

    for (i = 0u; i < snapshots->count; ++i) {
        clog_total += snapshots->items[i].inference.clog_risk;
        odor_total += snapshots->items[i].inference.odor_risk;
        freeze_total += snapshots->items[i].inference.freeze_risk;
    }

    printf("\nDrainVeil summary by %s\n", DRAINVEIL_AUTHOR);
    printf("  snapshots=%zu events=%zu\n", snapshots->count, events->count);
    if (snapshots->count > 0u) {
        printf("  avg_clog=%.2f avg_odor=%.2f avg_freeze=%.2f\n",
               clog_total / (float)snapshots->count,
               odor_total / (float)snapshots->count,
               freeze_total / (float)snapshots->count);
    }
    for (i = 0u; i < events->count; ++i) {
        printf("  event[%zu] minute=%u level=%s code=%s detail=%s\n",
               i,
               events->items[i].minute_index,
               dv_alert_name(events->items[i].level),
               events->items[i].code,
               events->items[i].detail);
    }
}
