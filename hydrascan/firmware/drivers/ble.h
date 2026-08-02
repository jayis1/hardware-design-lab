/*
 * drivers/ble.h — BLE GATT server (ANNA-B112 / Nordic UART + custom service)
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 */
#ifndef HYDRASCAN_BLE_H
#define HYDRASCAN_BLE_H
#include "../board.h"
#include "../classifier.h"

hydra_err_t ble_init(void);
/* Push the latest scan result to any connected client. */
void        ble_notify_result(const classify_result_t *r, float temp_c);
/* Poll for incoming library-write commands from the app. */
void        ble_poll(void);
uint8_t     ble_is_connected(void);
#endif