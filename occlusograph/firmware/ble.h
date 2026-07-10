/*
 * ble.h — BLE GATT server interface for Occlusograph.
 *
 * Author: jayis1
 * Copyright © 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef OCCLUSOGRAPH_BLE_H
#define OCCLUSOGRAPH_BLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "classify.h"

/* Initialize BLE stack and GATT services. */
int  ble_init(void);

/* Start advertising. */
void ble_advertise_start(void);
void ble_advertise_stop(void);

/* Push an event record out over the Event Log characteristic (notify). */
void ble_notify_event(const event_record_t *evt);

/* Flush coalesced event notifications (call periodically). */
void ble_flush_events(void);

/* Push a compressed force burst out over the Force Stream characteristic. */
void ble_notify_force(const int16_t force_mN[64], uint32_t timestamp);

/* Returns true if a central is currently connected. */
bool ble_is_connected(void);

/* Set a callback for when a connection is established / dropped. */
typedef void (*ble_conn_cb_t)(bool connected);
void ble_set_conn_callback(ble_conn_cb_t cb);

/* Set a callback for when the app writes the Calibration characteristic. */
typedef void (*ble_calib_cb_t)(uint8_t element, int16_t offset_mN);
void ble_set_calib_callback(ble_calib_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* OCCLUSOGRAPH_BLE_H */