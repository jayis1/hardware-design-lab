/* Author: jayis1 */
#ifndef SPLINTSENSE_LOGGER_H
#define SPLINTSENSE_LOGGER_H

#include <stddef.h>
#include "../board.h"

typedef struct {
    alert_event_t alerts[SPLINTSENSE_ALERT_LOG_CAPACITY];
    size_t count;
} event_log_t;

typedef struct {
    recovery_snapshot_t history[SPLINTSENSE_HISTORY_CAPACITY];
    size_t count;
} snapshot_log_t;

void logger_init(event_log_t *events, snapshot_log_t *snapshots);
void logger_push_event(event_log_t *events, uint32_t minute_index, alert_level_t level, const char *code, const char *detail);
void logger_push_snapshot(snapshot_log_t *snapshots, const recovery_snapshot_t *snapshot);
void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots);

#endif
