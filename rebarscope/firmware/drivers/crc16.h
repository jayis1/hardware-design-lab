/*
 * drivers/crc16.h — CRC-16 interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_CRC16_H
#define REBARSCOPE_CRC16_H

#include <stdint.h>

uint16_t crc16_compute(const uint8_t *data, uint8_t len);

#endif /* REBARSCOPE_CRC16_H */