/*
 * ble.h — BLE driver header
 * BreathPrint — Portable Breath VOC Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include <string.h>
#include "../board.h"

typedef enum {
    BLE_STATE_RESET = 0,
    BLE_STATE_INITIALIZED,
    BLE_STATE_READY,
    BLE_STATE_ADVERTISING,
    BLE_STATE_CONNECTED,
    BLE_STATE_ERROR
} ble_state_t;

void ble_init(void);
void ble_setup_gatt(void);
void ble_start_advertising(void);
void ble_stop_advertising(void);
void ble_poll(void);
uint8_t ble_get_command(void);
uint32_t ble_get_time(void);
void ble_notify_sample(const sensor_raw_t *sample, uint16_t index);
void ble_notify_result(const breath_result_t *result);
void ble_notify_status(uint8_t state, uint8_t battery, uint8_t charging);
uint8_t ble_is_connected(void);
ble_state_t ble_get_state(void);
int ble_send_log_data(const uint8_t *data, uint16_t len);

#endif /* BLE_H */