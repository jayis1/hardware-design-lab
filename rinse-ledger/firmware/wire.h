/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RINSE_WIRE_H
#define RINSE_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity);
#endif
