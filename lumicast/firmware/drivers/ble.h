/*
 * ble.h — BLE advertising & GATT server
 * LumiCast — Open-Source Circadian Light & SPD Analyzer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef LUMICAST_BLE_H
#define LUMICAST_BLE_H

#include <stdint.h>
#include <stdbool.h>
#include "circadian.h"
#include "spectrometer.h"
#include "flicker.h"

/* GATT characteristic IDs. */
#define BLE_CHAR_MELANOPIC_EDI    0x01
#define BLE_CHAR_ILLUMINANCE      0x02
#define BLE_CHAR_CCT              0x03
#define BLE_CHAR_CIRCADIAN_CS     0x04
#define BLE_CHAR_SPECTRUM         0x05
#define BLE_CHAR_FLICKER          0x06
#define BLE_CHAR_STATUS           0x07
#define BLE_CHAR_COMMAND          0x08
#define BLE_CHAR_TIMESTAMP        0x09

typedef enum {
    BLE_CMD_START_LOG = 0x01,
    BLE_CMD_STOP_LOG = 0x02,
    BLE_CMD_CALIBRATE = 0x03,
    BLE_CMD_SET_GAIN = 0x04,
    BLE_CMD_SET_ATIME = 0x05,
    BLE_CMD_STREAM_SPECTRUM = 0x06,
    BLE_CMD_SET_TIME = 0x07,
} ble_cmd_t;

typedef void (*ble_cmd_cb_t)(uint8_t cmd, const uint8_t *data, uint8_t len);

void ble_init(void);
void ble_start_advertising(void);
void ble_stop_advertising(void);
void ble_update_measurements(const circadian_result_t *circ,
                              const spectral_sample_t *spec,
                              const flicker_result_t *flk);
void ble_notify_subscribers(void);
void ble_set_command_callback(ble_cmd_cb_t cb);
bool ble_is_connected(void);
void ble_tick(void);

#endif /* LUMICAST_BLE_H */