/*
 * SplintSense logger
 * Author: jayis1
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
    alert_event_t *slot;
    if (events->count >= SPLINTSENSE_ALERT_LOG_CAPACITY) {
        return;
    }
    slot = &events->alerts[events->count++];
    slot->minute_index = minute_index;
    slot->level = level;
    snprintf(slot->code, sizeof(slot->code), "%s", code);
    snprintf(slot->detail, sizeof(slot->detail), "%s", detail);
}

void logger_push_snapshot(snapshot_log_t *snapshots, const recovery_snapshot_t *snapshot)
{
    if (snapshots->count >= SPLINTSENSE_HISTORY_CAPACITY) {
        return;
    }
    snapshots->history[snapshots->count++] = *snapshot;
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    size_t i;
    printf("\n=== SplintSense Event Summary (author: jayis1) ===\n");
    printf("Snapshots captured: %zu\n", snapshots->count);
    printf("Alert events: %zu\n", events->count);
    for (i = 0u; i < events->count; ++i) {
        printf("  - t=%u min level=%u code=%s detail=%s\n",
               events->alerts[i].minute_index,
               (unsigned)events->alerts[i].level,
               events->alerts[i].code,
               events->alerts[i].detail);
    }
}
