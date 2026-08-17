/*
 * ble.h — BLE 5.2 Communication via nRF52833 Module
 *
 * LignoScan — Portable Acoustic Tomography Scanner
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef LIGNOSCAN_BLE_H
#define LIGNOSCAN_BLE_H

#include <stdint.h>

/* BLE state codes (sent in status packets) */
#define BLE_STATE_IDLE         0x00
#define BLE_STATE_CALIBRATING  0x01
#define BLE_STATE_SCANNING     0x02
#define BLE_STATE_RECONSTRUCT  0x03
#define BLE_STATE_TRANSMITTING 0x04
#define BLE_STATE_ERROR        0xFF

/* Packet types */
#define BLE_PKT_STATUS    0x01
#define BLE_PKT_TOF       0x02
#define BLE_PKT_TOMOGRAM  0x03
#define BLE_PKT_GPS       0x04
#define BLE_PKT_INFO      0x05
#define BLE_PKT_SCAN_REQ  0x06

/* GPS fix structure (shared with GPS driver) */
typedef struct {
    float latitude;
    float longitude;
    float altitude_m;
    float hdop;
    int fix_quality;    /* 0 = no fix, 1 = GPS, 2 = DGPS */
    int satellites;
    char timestamp[24]; /* ISO 8601 format */
} gps_fix_t;

void ble_init(void);
void ble_send_status(uint8_t state, uint32_t progress);
void ble_send_tof_matrix(float *matrix, int n_sensors);
void ble_send_tomogram(float *velocity, uint8_t *classification, int n_cells);
void ble_send_gps(gps_fix_t *fix);
void ble_send_device_info(void);
int ble_receive_command(uint8_t *cmd, uint8_t *data, int maxlen);
int ble_is_connected(void);

#endif /* LIGNOSCAN_BLE_H */