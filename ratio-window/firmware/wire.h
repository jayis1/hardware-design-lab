/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RATIO_WIRE_H
#define RATIO_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *text, size_t length);
size_t wire_encode(const Engine *e, char *out, size_t capacity);
#endif
