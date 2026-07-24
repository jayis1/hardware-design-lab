/*
 * ble_drv.h — BLE UART driver for BMD-300 module
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef ALLOYSCAN_BLE_DRV_H
#define ALLOYSCAN_BLE_DRV_H

#include <stdint.h>
#include <stdbool.h>

#define BLE_TX_BUF_SIZE   512
#define BLE_RX_BUF_SIZE   256

/* Initialize USART2 for BLE communication */
void ble_drv_init(void);

/* Send data over BLE (non-blocking, returns bytes sent) */
uint32_t ble_drv_send(const uint8_t *data, uint32_t len);

/* Send a null-terminated string over BLE */
void ble_drv_send_string(const char *str);

/* Check if BLE is connected */
bool ble_drv_is_connected(void);

/* Read received data (returns bytes read, 0 if nothing available) */
uint32_t ble_drv_receive(uint8_t *buffer, uint32_t max_len);

/* Set BLE connection state (called by GPIO interrupt or command parser) */
void ble_drv_set_connected(bool connected);

/* Process BLE commands (call from main loop) */
void ble_drv_process(void);

/* Send a scan result as JSON */
void ble_drv_send_scan_result(const char *alloy_name, float confidence,
                               const char *alts[], int alt_count,
                               float liftoff_mm);

#endif /* ALLOYSCAN_BLE_DRV_H */