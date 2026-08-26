/*
 * PipeWhisper logger
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

void logger_push_event(event_log_t *events, uint32_t minute, alert_level_t level, const char *code, const char *detail)
{
    pipe_event_t *event;
    if (events->count >= PIPEWHISPER_EVENT_CAPACITY) return;
    event = &events->events[events->count++];
    event->minute_index = minute;
    event->level = level;
    snprintf(event->code, sizeof(event->code), "%s", code);
    snprintf(event->detail, sizeof(event->detail), "%s", detail);
}

void logger_push_snapshot(snapshot_log_t *snapshots, const pipe_snapshot_t *snapshot)
{
    if (snapshots->count >= PIPEWHISPER_HISTORY_CAPACITY) return;
    snapshots->history[snapshots->count++] = *snapshot;
}

void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots)
{
    size_t i;
    float leak_peak = 0.0f, freeze_peak = 0.0f, hammer_peak = 0.0f;
    for (i = 0u; i < snapshots->count; ++i) {
        const pipe_snapshot_t *snapshot = &snapshots->history[i];
        if (snapshot->inference.leak_confidence > leak_peak) leak_peak = snapshot->inference.leak_confidence;
        if (snapshot->inference.freeze_risk > freeze_peak) freeze_peak = snapshot->inference.freeze_risk;
        if (snapshot->pressure.hammer_score > hammer_peak) hammer_peak = snapshot->pressure.hammer_score;
    }
    printf("\nEvent summary by jayis1\n");
    printf("  Snapshots captured: %zu\n", snapshots->count);
    printf("  Logged events: %zu\n", events->count);
    printf("  Peak leak confidence: %.2f\n", leak_peak);
    printf("  Peak freeze risk: %.2f\n", freeze_peak);
    printf("  Peak hammer score: %.2f\n", hammer_peak);
    for (i = 0u; i < events->count; ++i) {
        const pipe_event_t *event = &events->events[i];
        printf("  - m%02u [%u] %s :: %s\n", event->minute_index, (unsigned)event->level, event->code, event->detail);
    }
}
