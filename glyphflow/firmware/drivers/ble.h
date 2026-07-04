/*
 * ble.h — BLE module transport (BGM220P) over UART
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 */
#ifndef GLYPHFLOW_BLE_H
#define GLYPHFLOW_BLE_H

#include <stdint.h>

/* A standard USB HID keyboard report. */
typedef struct {
    uint8_t modifiers;   /* Ctrl/Shift/Alt/GUI bits */
    uint8_t reserved;
    uint8_t keys[6];     /* up to 6 simultaneous keys, 0 = none */
} hid_report_t;

int ble_init(void);
int ble_advertise_start(void);
int ble_advertise_stop(void);
int ble_send_hid(const hid_report_t *r);
int ble_send_char(uint8_t hid_scancode, uint8_t modifiers);
int ble_notify_trajectory(const int16_t *pts, uint8_t n);
int ble_send_battery(uint8_t percent);
int ble_send_status(const char *msg, uint8_t len);
int ble_set_active_set(uint8_t set_mask);
int ble_sleep(void);
int ble_wake(void);

/* Returns 1 if connected, 0 if not. */
int ble_is_connected(void);

#endif /* GLYPHFLOW_BLE_H */