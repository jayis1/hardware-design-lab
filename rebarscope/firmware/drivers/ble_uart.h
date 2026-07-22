/*
 * drivers/ble_uart.h — BLE UART interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_BLE_UART_H
#define REBARSCOPE_BLE_UART_H

#include <stdint.h>

typedef void (*ble_rx_cb_t)(const uint8_t *data, uint8_t len);

void    ble_uart_init(ble_rx_cb_t cb);
int     ble_uart_send(const uint8_t *payload, uint8_t len);
void    ble_uart_poll(void);
void    ble_uart_set_connected(uint8_t connected);
uint8_t ble_uart_is_connected(void);

#endif /* REBARSCOPE_BLE_UART_H */