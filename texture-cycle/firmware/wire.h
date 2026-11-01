/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TEXTURE_WIRE_H
#define TEXTURE_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t length);
size_t wire_encode(const Engine *e, char *out, size_t capacity);
#endif
