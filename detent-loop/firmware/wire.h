/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef DETENT_WIRE_H
#define DETENT_WIRE_H
#include "engine.h"
#include <stddef.h>
uint16_t wire_crc(const char *bytes,size_t length);
size_t wire_encode(const Engine *e,uint32_t sequence,char *out,size_t capacity);
#endif
