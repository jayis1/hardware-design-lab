/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef RA_WIRE_H
#define RA_WIRE_H
#include <stddef.h>
#include "engine.h"
size_t wire_encode(const Engine *e, char *out, size_t capacity);
#endif
