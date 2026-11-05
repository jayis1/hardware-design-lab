/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef FLAP_WIRE_H
#define FLAP_WIRE_H
#include "engine.h"
#include <stddef.h>
size_t wire_encode(const Engine *e, char *destination, size_t capacity);
#endif
