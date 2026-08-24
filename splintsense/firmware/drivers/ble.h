/* Author: jayis1 */
#ifndef SPLINTSENSE_BLE_H
#define SPLINTSENSE_BLE_H

#include <stddef.h>
#include "../board.h"

size_t ble_build_status_packet(const recovery_snapshot_t *snapshot, char *buffer, size_t capacity);
size_t ble_build_clinician_packet(const recovery_snapshot_t *snapshot, const alert_event_t *events, size_t event_count, char *buffer, size_t capacity);

#endif
