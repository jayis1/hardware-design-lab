/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CROSSOVER_WIRE_H
#define CROSSOVER_WIRE_H
#include <stddef.h>
#include <stdint.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t length);
size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity);
#endif
