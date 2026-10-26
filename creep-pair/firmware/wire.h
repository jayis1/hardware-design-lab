/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CREEP_WIRE_H
#define CREEP_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t wire_crc(const char *data,size_t length);
size_t wire_encode(const Engine *e,uint32_t sequence,char *out,size_t capacity);
#endif
