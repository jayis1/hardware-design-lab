/*
 * ble.h — BLE UART protocol interface (nRF52840)
 * FerroProbe — Open-Source 3-Axis Fluxgate Magnetometer
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef FERROPROBE_BLE_H
#define FERROPROBE_BLE_H

#include <stdint.h>
#include "lockin.h"
#include "gps.h"
#include "imu.h"

/* Initialize UART4 for communication with nRF52840 BLE module */
void ble_init(void);

/* Send a status response frame */
void ble_send_status(uint8_t battery_pct, uint8_t gps_fix,
                      uint8_t mode, uint8_t calib_valid,
                      uint32_t record_count, int8_t temp_c);

/* Send a field measurement data frame (streaming mode) */
void ble_send_field(const field_measurement_t *field,
                     const gps_data_t *gps, uint8_t flags);

/* Send a calibration data frame */
void ble_send_calib(const float offset[3], const float scale[3],
                     const float cross[3][3]);

/* Send a log data chunk (for download) */
void ble_send_log_chunk(uint32_t offset, const uint8_t *data, uint8_t len);

/* Send a version response */
void ble_send_version(void);

/* Send a simple OK response */
void ble_send_ok(uint8_t opcode);

/* Send an error response */
void ble_send_error(uint8_t opcode, uint8_t error_code);

/* Process incoming BLE commands (call from main loop) */
void ble_process(void);

/* Set the callback for mode change requests */
typedef void (*ble_mode_callback_t)(uint8_t mode);
void ble_set_mode_callback(ble_mode_callback_t cb);

/* Set the callback for threshold changes */
typedef void (*ble_threshold_callback_t)(int32_t threshold_nt);
void ble_set_threshold_callback(ble_threshold_callback_t cb);

/* Set the callback for rate changes */
typedef void (*ble_rate_callback_t)(uint8_t rate);
void ble_set_rate_callback(ble_rate_callback_t cb);

#endif /* FERROPROBE_BLE_H */