/*
 * ble_drv.h — BLE GATT service driver header
 * TremorSense — Wearable Tremor Characterization Band
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: MIT
 */
#ifndef TREMORSENSE_BLE_DRV_H
#define TREMORSENSE_BLE_DRV_H

#include <stdint.h>
#include <stddef.h>

int   ble_init(void);
void  ble_start_advertising(uint16_t interval_ms, int8_t tx_power_dbm);
void  ble_stop_advertising(void);
int   ble_is_connected(void);
void  ble_disconnect(void);
void  ble_notify_tremor_status(const uint8_t *data, uint8_t len);
int   ble_send_episode_chunk(const uint8_t *data, uint16_t len);
int   ble_start_download(void);
void  ble_set_device_name(const char *name);

/* Config write callback */
typedef void (*ble_config_callback_t)(uint16_t uuid, const uint8_t *data,
                                        uint16_t len);
void ble_register_config_callback(ble_config_callback_t cb);

/* NFC callback for medication dose logging */
typedef void (*nfc_tag_callback_t)(void);
void ble_register_nfc_callback(nfc_tag_callback_t cb);
void ble_log_medication_dose(uint32_t timestamp);

#endif /* TREMORSENSE_BLE_DRV_H */