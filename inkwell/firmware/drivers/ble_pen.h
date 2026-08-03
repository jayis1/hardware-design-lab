/*
 * ble_pen.h — BLE 5.0 GATT + HID Pen service for Inkwell
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */
#ifndef INKWELL_DRIVERS_BLE_PEN_H
#define INKWELL_DRIVERS_BLE_PEN_H

#include <stdint.h>
#include <stdbool.h>
#include "stroke.h"

void ble_pen_init(void);
void ble_pen_notify_segment(const stroke_segment_t *seg);
void ble_pen_notify_status(uint8_t battery_pct, uint8_t power_state, uint8_t flash_pct);
bool ble_pen_is_connected(void);
void ble_pen_start_session(void);
void ble_pen_stop_session(void);
void ble_pen_set_segment_rate(uint16_t ms);

#endif