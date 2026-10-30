/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TEARDOWN_WIRE_H
#define TEARDOWN_WIRE_H
#include "engine.h"
#include <stddef.h>
size_t wire_encode(const Engine *e, char *output, size_t capacity);
#endif
