/* Author: jayis1 */
#ifndef PIPEWHISPER_BLE_H
#define PIPEWHISPER_BLE_H

#include "../board.h"

size_t ble_build_status_packet(const pipe_snapshot_t *snapshot, char *buffer, size_t capacity);
size_t ble_build_report_packet(const pipe_snapshot_t *snapshot, const event_log_t *events, char *buffer, size_t capacity);

#endif
