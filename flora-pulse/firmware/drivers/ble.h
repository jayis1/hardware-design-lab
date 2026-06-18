/*
 * ble.h — nRF52840 BLE module communication driver
 * FloraPulse — Plant Electrophysiology & Sap-Flow Stress Monitor
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FLORAPULSE_BLE_H
#define FLORAPULSE_BLE_H

#include <stdint.h>

/* Initialize USART1 for BLE module communication (115200 8N1).
 * The nRF52840 runs a UART-to-BLE-透明传输 bridge firmware.
 */
void ble_init(void);

/* Send a complete framed message to the BLE module.
 * Frame: [SOF][LEN][OPCODE][PAYLOAD...][CRC][EOF]
 * Returns 0 on success.
 */
uint8_t ble_send(uint8_t opcode, const uint8_t *payload, uint8_t len);

/* Poll for incoming data from BLE module.
 * Call from main loop. Assembles frames and invokes handler.
 * Returns 1 if a complete frame was received, 0 otherwise.
 */
uint8_t ble_poll(void);

/* Set the callback for received commands */
typedef void (*ble_cmd_handler_t)(uint8_t opcode, const uint8_t *payload, uint8_t len);
void ble_set_handler(ble_cmd_handler_t handler);

/* Stream real-time data (sample or waveform) to the app */
void ble_stream_sample(const uint8_t *data, uint8_t len);

/* Check if BLE is connected (from module status pin) */
uint8_t ble_is_connected(void);

/* Send log chunk for download */
void ble_send_log_chunk(uint32_t offset, const uint8_t *data, uint8_t len);

#endif /* FLORAPULSE_BLE_H */