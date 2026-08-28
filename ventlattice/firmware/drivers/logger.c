/*
 * VentLattice logger
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

void logger_push_event(event_log_t *events, uint32_t hour_index, alert_level_t level, const char *code, const char *detail)
{
    vent_event_t *item;
    if (events->count >= VENTLATTICE_EVENT_CAPACITY) return;
    item = &events->items[events->count++];
    item->hour_index = hour_index;
    item->level = level;
    snprintf(item->code, sizeof(item->code), "%s", code);
    snprintf(item->detail, sizeof(item->detail), "%s", detail);
}

void logger_push_snapshot(snapshot_log_t *snapshots, const vent_snapshot_t *snapshot)
{
    if (snapshots->count >= VENTLATTICE_SNAPSHOT_CAPACITY) return;
    snapshots->items[snapshots->count++] = *snapshot;
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    size_t i;
    printf("\nEvent summary (%zu events)\n", events->count);
    for (i = 0; i < events->count; ++i) {
        printf("  [%02u] level=%u code=%s detail=%s\n",
               (unsigned)events->items[i].hour_index,
               (unsigned)events->items[i].level,
               events->items[i].code,
               events->items[i].detail);
    }
    printf("\nStored snapshots: %zu\n", snapshots->count);
    if (snapshots->count > 0u) {
        const vent_snapshot_t *first = &snapshots->items[0];
        const vent_snapshot_t *last = &snapshots->items[snapshots->count - 1u];
        printf("  first_service=%.1f last_service=%.1f first_cfm=%.1f last_cfm=%.1f\n",
               first->inference.service_score,
               last->inference.service_score,
               first->airflow.airflow_cfm,
               last->airflow.airflow_cfm);
    }
}
