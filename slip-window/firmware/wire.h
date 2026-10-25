/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#ifndef SLIP_WIRE_H
#define SLIP_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t length);
size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity);
#endif
