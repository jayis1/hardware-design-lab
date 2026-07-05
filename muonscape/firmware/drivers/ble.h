/*
 * ble.h — BLE 5.2 status + command interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_BLE_H
#define MUONSCAPE_DRV_BLE_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GATT service UUIDs */
#define BLE_SVC_UUID            0x181A  /* environmental service base */
#define BLE_CHAR_STATUS_UUID    0x2A6E
#define BLE_CHAR_CMD_UUID       0x2A6F
#define BLE_CHAR_TRACK_UUID     0x2A70

muon_status_t ble_init(void);
void          ble_set_status(const char *json, uint16_t len);
void          ble_poll(void);   /* service the BLE stack, called from main loop */
uint16_t      ble_get_cmd(char *buf, uint16_t max);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_BLE_H */