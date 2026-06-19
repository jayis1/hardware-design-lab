/*
 * ble.h — nRF52840 BLE 5.2 Protocol Driver
 * StrataScan — Open-Source Stepped-Frequency Ground Penetrating Radar
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef STRATASCAN_BLE_H
#define STRATASCAN_BLE_H

#include <stdint.h>

int   ble_init(void);
uint8_t ble_get_command(void);
uint8_t ble_get_param_byte(void);
uint32_t ble_get_param_word(void);
int   ble_send_packet(const uint8_t *data, uint16_t len);
int   ble_send_bscan_row(const float *trace, const float *depth,
                          uint16_t n, uint16_t row);
void  ble_notify_trace_ready(uint32_t trace_num);

/* Command protocol opcodes */
#define BLE_CMD_START_SURVEY    0x01
#define BLE_CMD_PAUSE_SURVEY   0x02
#define BLE_CMD_STOP_SURVEY    0x03
#define BLE_CMD_RECALIBRATE    0x04
#define BLE_CMD_SET_BAND       0x05
#define BLE_CMD_SET_EPS_R      0x06
#define BLE_CMD_SET_SPACING    0x07
#define BLE_CMD_GET_STATUS     0x08
#define BLE_CMD_GET_BSCAN_ROW  0x09
#define BLE_CMD_SHUTDOWN       0x0A

#endif /* STRATASCAN_BLE_H */