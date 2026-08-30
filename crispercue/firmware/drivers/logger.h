/*
 * CrisperCue logging interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_LOGGER_H
#define CRISPERCUE_LOGGER_H

#include "../board.h"

void logger_init(event_log_t *events, snapshot_log_t *snapshots);
void logger_push_event(event_log_t *events, uint32_t cycle, alert_level_t level, const char *code, const char *detail);
void logger_push_snapshot(snapshot_log_t *snapshots, const crisper_snapshot_t *snapshot);
void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots);

#endif
