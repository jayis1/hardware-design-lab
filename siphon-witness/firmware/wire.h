/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef SW_WIRE_H
#define SW_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t length);
size_t wire_encode(const Engine *e, uint32_t now, uint32_t sequence,
                   char *output, size_t capacity);
#endif
