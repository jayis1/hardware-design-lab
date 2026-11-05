/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef DETENT_WIRE_H
#define DETENT_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_encode(Engine *e, char *out, size_t capacity);
#endif
