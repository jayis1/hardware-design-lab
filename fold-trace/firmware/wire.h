/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FOLD_WIRE_H
#define FOLD_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *data, size_t length);
size_t wire_status(char *out, size_t capacity, const Engine *e,
                   uint32_t now, uint32_t sequence, uint32_t transport_loss);
#endif
