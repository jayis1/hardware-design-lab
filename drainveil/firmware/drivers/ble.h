/*
 * DrainVeil BLE protocol helpers
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 */
#ifndef DRAINVEIL_BLE_H
#define DRAINVEIL_BLE_H

#include "../board.h"

size_t ble_build_status_packet(const drain_snapshot_t *snapshot, char *buffer, size_t buffer_size);
size_t ble_build_report_packet(const drain_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t buffer_size);

#endif
