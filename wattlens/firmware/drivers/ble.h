/*
 * ble.h — BLE communication via nRF52840
 * WattLens — 3-Phase Power Quality Analyzer & NILM
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: MIT
 */

#ifndef WATTLENS_BLE_H
#define WATTLENS_BLE_H

#include "board.h"

void ble_init(void);
void ble_send_metrics(const power_metrics_t *m, const nilm_result_t *n);
void ble_send_harmonics(const harmonic_data_t *h);
void ble_send_event(const pq_event_t *evt);
void ble_send_status(const device_ctx_t *ctx);
void ble_poll_commands(device_ctx_t *ctx);

/* BLE command types (received from app) */
typedef enum {
    BLE_CMD_START = 0x01,
    BLE_CMD_STOP = 0x02,
    BLE_CMD_SET_CT = 0x03,
    BLE_CMD_SET_GRID = 0x04,
    BLE_CMD_CALIBRATE = 0x05,
    BLE_CMD_GET_HARMONICS = 0x06,
    BLE_CMD_GET_EVENTS = 0x07,
    BLE_CMD_GET_STATUS = 0x08,
    BLE_CMD_UPLOAD_MODEL = 0x09,
    BLE_CMD_SET_NAME = 0x0A,
    BLE_CMD_CAPTURE = 0x0B,
} ble_cmd_t;

#endif /* WATTLENS_BLE_H */