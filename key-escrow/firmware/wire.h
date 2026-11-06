/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ESCROW_WIRE_H
#define ESCROW_WIRE_H
#include <stddef.h>
#include <stdint.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t size);
size_t wire_encode(const Engine *e, char *buffer, size_t capacity);
#endif
