/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef GRIP_WIRE_H
#define GRIP_WIRE_H
#include <stddef.h>
#include "engine.h"
size_t wire_encode(const Engine *e, char *destination, size_t capacity);
#endif
