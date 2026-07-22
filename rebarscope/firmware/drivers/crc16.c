/*
 * drivers/crc16.c — CRC-16/CCITT-FALSE for BLE frame integrity
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "crc16.h"
#include <stdint.h>

uint16_t crc16_compute(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFFu;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000u) crc = (crc << 1) ^ 0x1021u;
            else               crc <<= 1;
        }
    }
    return crc;
}