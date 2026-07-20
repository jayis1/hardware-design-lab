/*
 * ble_proto.h — BLE Protocol (header)
 * RootTrace — Open-Source Electrical Impedance Tomography Root Imaging System
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef ROOTTRACE_BLE_PROTO_H
#define ROOTTRACE_BLE_PROTO_H

#include <stdint.h>
#include "eit_acq.h"

/* BLE command packet (received from companion app) */
typedef struct {
    uint8_t opcode;
    uint8_t seq;
    uint16_t length;
    uint8_t payload[240];
} ble_cmd_t;

/* BLE response packet (sent to companion app) */
typedef struct {
    uint8_t opcode;
    uint8_t status;
    uint8_t seq;
    uint16_t length;
    uint8_t payload[BLE_MAX_PACKET - 6];
} ble_resp_t;

void ble_proto_init(void);
void ble_proto_process(void);
int  ble_proto_send_response(const ble_resp_t *resp);
int  ble_proto_send_frame(const eit_frame_t *frame);
int  ble_proto_send_image(const float *image, int w, int h);
int  ble_proto_is_connected(void);
void ble_proto_reset_co_processor(void);

/* Command handlers (called by main state machine) */
typedef int (*ble_cmd_handler_t)(const ble_cmd_t *cmd, ble_resp_t *resp);
void ble_proto_register_handler(uint8_t opcode, ble_cmd_handler_t handler);

#endif /* ROOTTRACE_BLE_PROTO_H */