/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ATLAS_WIRE_H
#define ATLAS_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t length);
size_t wire_status(char *out, size_t capacity, const Atlas *a,
                   uint32_t now, uint32_t sequence, uint32_t loss);
size_t wire_event(char *out, size_t capacity, const AtlasEvent *event,
                  uint32_t sequence);
#endif
