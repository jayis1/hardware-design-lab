/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CW_WIRE_H
#define CW_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t cw_crc(const char *text, size_t size);
size_t cw_encode(const CW *e, uint32_t now, uint32_t tx_errors,
                 char *output, size_t capacity);
#endif
