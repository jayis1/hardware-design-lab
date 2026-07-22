/*
 * drivers/cobs.h — COBS codec interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_COBS_H
#define REBARSCOPE_COBS_H

#include <stdint.h>

uint8_t cobs_encode(const uint8_t *in, uint8_t len, uint8_t *out);
uint8_t cobs_decode(const uint8_t *in, uint8_t len, uint8_t *out);

#endif /* REBARSCOPE_COBS_H */