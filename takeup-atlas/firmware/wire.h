/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TAKEUP_WIRE_H
#define TAKEUP_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t size);
size_t wire_encode(const Engine *engine, uint32_t sequence, uint32_t transport_errors,
                   char *buffer, size_t capacity);
#endif
