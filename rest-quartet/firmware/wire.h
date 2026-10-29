/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef REST_WIRE_H
#define REST_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *text, size_t length);
size_t wire_encode(const Engine *e, char *buffer, size_t capacity);
#endif
