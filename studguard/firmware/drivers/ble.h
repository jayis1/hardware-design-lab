/*
 * ble.h — StudGuard BLE telemetry encoder
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef STUDGUARD_BLE_H
#define STUDGUARD_BLE_H

#include <stddef.h>
#include "../board.h"

void ble_init(void);
void ble_encode_status(const sg_device_status_t *status, const sg_measurement_t *measurement, char *buffer, size_t buffer_len);
void ble_encode_peers(const sg_peer_snapshot_t *peers, size_t count, char *buffer, size_t buffer_len);

#endif
