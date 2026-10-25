/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RC_WIRE_H
#define RC_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t size);
size_t wire_encode(const Engine *e, uint32_t sequence, uint32_t errors,
                   char *buffer, size_t capacity);
#endif
