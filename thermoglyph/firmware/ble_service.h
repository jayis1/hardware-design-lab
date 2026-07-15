/*
 * ble_service.h — BLE GATT service for ThermoGlyph
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * License: GPL-3.0
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "glyph_engine.h"

/* Initialize BLE stack and GATT service */
bool ble_init(void);

/* Process BLE events (called from comm_task loop) */
void ble_process(void);

/* Send gesture notification to connected client */
bool ble_notify_gesture(uint8_t gesture_code);

/* Send telemetry notification */
bool ble_notify_telemetry(const telemetry_packet_t *tel);

/* Check if a client is connected */
bool ble_is_connected(void);

/* Get the last received glyph command (from BLE write) */
bool ble_get_glyph_cmd(glyph_cmd_t *cmd);

/* Broadcast: send glyph to all connected peers (for mesh relay) */
void ble_set_glyph_callback(void (*cb)(const glyph_cmd_t *cmd));

#endif /* BLE_SERVICE_H */