/*
 * CrisperCue BLE packet builder interface
 * Author: jayis1
 */
#ifndef CRISPERCUE_BLE_H
#define CRISPERCUE_BLE_H

#include "../board.h"

size_t ble_build_status_packet(const crisper_snapshot_t *snapshot, char *buffer, size_t size);
size_t ble_build_report_packet(const crisper_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t size);

#endif
