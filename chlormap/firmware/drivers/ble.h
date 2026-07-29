/*
 * ble.h — NINA-B306 BLE 5.0 UART driver interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef DRIVERS_BLE_H
#define DRIVERS_BLE_H

#include <stdint.h>
#include <stdbool.h>

/* BLE status (sent via status characteristic) */
typedef struct {
    uint16_t batt_mv;
    uint8_t  state;
    uint8_t  sats;
    uint8_t  fix_type;
} ble_status_t;

/* Initialize BLE module (UART + NCP protocol) */
bool ble_init(void);

/* Start advertising with given device name */
bool ble_start_advertising(const char *name);

/* Stop advertising */
bool ble_stop_advertising(void);

/* Send measurement notification (48-byte binary packet) */
bool ble_send_notification(const uint8_t *data, uint16_t len);

/* Send status characteristic update */
bool ble_send_status(const ble_status_t *status);

/* Check if a client is connected */
bool ble_is_connected(void);

/* Poll BLE module for events (call from main loop) */
void ble_poll(void);

/* Disconnect current client */
bool ble_disconnect(void);

/* Get connection state */
uint8_t ble_get_state(void);

/* Set TX power (-40 to +8 dBm) */
bool ble_set_tx_power(int8_t dbm);

#endif /* DRIVERS_BLE_H */