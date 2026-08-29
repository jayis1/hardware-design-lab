/*
 * DrainVeil logger helpers
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_LOGGER_H
#define DRAINVEIL_LOGGER_H

#include "../board.h"

void logger_init(event_log_t *events, snapshot_log_t *snapshots);
void logger_push_event(event_log_t *events, uint32_t minute_index, alert_level_t level, const char *code, const char *detail);
void logger_push_snapshot(snapshot_log_t *snapshots, const drain_snapshot_t *snapshot);
void logger_print_summary(const event_log_t *events, const snapshot_log_t *snapshots);

#endif
