/*
 * ble.h — BLE co-processor protocol (UART to BL654 module)
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef LITHOCORE_BLE_H
#define LITHOCORE_BLE_H

#include <stdint.h>
#include "../board.h"
#include "soh.h"

/* BLE GATT service UUID */
#define BLE_SERVICE_UUID  "6e400001-b5a3-f393-e0a9-e50e24dcca9e"

/* Characteristic UUIDs (suffixes) */
#define BLE_CHAR_CMD      0x0002  /* phone → device: commands */
#define BLE_CHAR_DATA     0x0003  /* device → phone: sweep data points */
#define BLE_CHAR_RESULT   0x0004  /* device → phone: final result */
#define BLE_CHAR_STATUS   0x0005  /* device → phone: status updates */

/* Command bytes (phone → device) */
#define BLE_CMD_START_FAST   0x01
#define BLE_CMD_START_FULL   0x02
#define BLE_CMD_ABORT        0x03
#define BLE_CMD_GET_STATUS   0x04
#define BLE_CMD_GET_RESULT   0x05
#define BLE_CMD_GET_HISTORY  0x06
#define BLE_CMD_SET_CONFIG   0x07
#define BLE_CMD_CALIBRATE    0x08

/* Return codes */
#define BLE_OK    0
#define BLE_ERROR -1
#define BLE_TIMEOUT -2

/* API */
int  ble_init(const litho_config_t *config);
int  ble_get_command(uint8_t *cmd);
int  ble_send_status(uint8_t state, uint8_t progress, uint8_t result_valid);
int  ble_send_sweep_point(uint32_t freq_hz, int32_t re_z, int32_t im_z,
                          int32_t mag, int32_t phase, uint8_t flags);
int  ble_send_result(const soh_result_t *result);
int  ble_send_error(const char *msg);
int  ble_receive_config(litho_config_t *config);
uint8_t ble_is_connected(void);
void ble_send_notification(const uint8_t *data, uint8_t len);

#endif /* LITHOCORE_BLE_H */