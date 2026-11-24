/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef ECHO_WIRE_H
#define ECHO_WIRE_H
#include <stddef.h>
#include <stdint.h>
#include "engine.h"
uint16_t wire_crc(const char *data, size_t length);
size_t wire_encode(const Engine *e, char *output, size_t capacity);
#endif
