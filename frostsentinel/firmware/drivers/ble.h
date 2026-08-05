/*
 * drivers/ble.h — BLE module driver header
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef FROSTSENTINEL_BLE_H
#define FROSTSENTINEL_BLE_H

#include <stdint.h>

/* Callback type for commands received from the app via BLE. */
typedef void (*ble_command_cb_t)(const uint8_t *payload, uint8_t len);

/* Initialize the BLE module (UART + reset). */
void ble_init(ble_command_cb_t command_callback);

/* Send live sensor data as a BLE notification (16-byte payload). */
void ble_send_live_data(void);

/* Send a 24-byte historical log record as a BLE notification. */
void ble_send_log_record(const uint8_t *record24);

/* Send a status response (12-byte payload). */
void ble_send_status(void);

/* Poll for incoming commands (non-blocking). Returns 1 if a command
 * was received and dispatched to the callback, 0 otherwise. */
int ble_poll(void);

/* Update the network AES key (called after provisioning). */
void ble_set_network_key(const uint8_t *key16);

#endif /* FROSTSENTINEL_BLE_H */