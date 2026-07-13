/*
 * ble.h — BLE 5.2 companion app interface for HivePulse
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include <stdbool.h>
#include "sensors.h"

/* BLE commands from the phone app */
typedef enum {
    BLE_CMD_NONE = 0,
    BLE_CMD_REQUEST_AUDIO = 1,       /* Request live audio stream */
    BLE_CMD_REQUEST_SENSORS = 2,     /* Request current sensor data */
    BLE_CMD_TARE_WEIGHT = 3,         /* Tare load cells */
    BLE_CMD_SET_SCHEDULE = 4,        /* Set sampling schedule */
    BLE_CMD_OTA_UPDATE = 5,          /* Enter DFU mode for OTA firmware update */
    BLE_CMD_SET_GAIN = 6,            /* Set microphone gain */
    BLE_CMD_GET_STATE = 7,           /* Get current colony state + confidence */
    BLE_CMD_RECORD_SNAPSHOT = 8,     /* Record 30s audio clip to phone */
} ble_cmd_t;

typedef struct {
    ble_cmd_t type;
    uint8_t   param;
    uint8_t   data[16];
    uint8_t   data_len;
} ble_command_t;

/* Initialize BLE co-processor (CC2640R2F) over UART */
int ble_init(void);

/* BLE service routine (called periodically from main loop) */
void ble_service(void);

/* Check for received commands from the phone app */
bool ble_get_command(ble_command_t *cmd);

/* Send sensor data to phone */
void ble_send_sensor_data(const sensor_data_t *data);

/* Send colony state to phone */
void ble_send_colony_state(uint8_t state, float confidence);

/* Send audio level data to phone (for spectrogram display) */
void ble_send_audio_levels(const float levels[4]);

/* Enter DFU (Device Firmware Update) mode */
void ble_enter_dfu(void);

/* UART protocol functions */
int ble_uart_send(const uint8_t *data, int len);
int ble_uart_receive(uint8_t *buf, int len, int timeout_ms);

#endif /* BLE_H */