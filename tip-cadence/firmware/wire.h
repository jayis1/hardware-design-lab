/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TC_WIRE_H
#define TC_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_encode(const Engine *e, char *out, size_t capacity);
#endif
