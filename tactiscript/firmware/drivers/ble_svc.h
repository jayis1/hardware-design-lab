/*
 * ble_svc.h — TactiScript BLE GATT service header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef TACTISCRIPT_BLE_SVC_H
#define TACTISCRIPT_BLE_SVC_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize BLE stack and custom GATT service */
void ble_init(void);

/* Poll BLE events (non-blocking, call from main loop) */
void ble_poll(void);

/* Notify connected client of a detected gesture */
void ble_notify_gesture(uint8_t gesture);

/* Notify connected client of status update (battery, mode, etc.) */
void ble_notify_status(uint8_t battery_pct, bool charging,
                        uint8_t mode, bool connected);

/* Check if a client is currently connected */
bool ble_is_connected(void);

/* Disconnect current client */
void ble_disconnect(void);

/* --- Callbacks implemented by the application (main.c) --- */
/* These are called by the BLE stack when data arrives from the phone. */
void ble_on_text_received(const uint8_t *data, uint16_t len);
void ble_on_command_received(uint8_t cmd);
void ble_on_texture_received(const uint8_t *data, uint16_t len);
void ble_on_nav_received(uint8_t direction);
void ble_on_connected(void);
void ble_on_disconnected(void);

#endif /* TACTISCRIPT_BLE_SVC_H */