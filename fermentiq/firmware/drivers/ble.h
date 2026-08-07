/*
 * ble.h — BLE GATT Server Header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef FERMENTIQ_BLE_H
#define FERMENTIQ_BLE_H

#include "board.h"

/* API */
int ble_init(void);
void ble_process(void);
void ble_send_alert(const char *message, int severity);
void ble_notify_state(const fermentiq_state_t *state);

#endif /* FERMENTIQ_BLE_H */