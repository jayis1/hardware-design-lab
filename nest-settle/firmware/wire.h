/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef NS_WIRE_H
#define NS_WIRE_H
#include <stddef.h>
#include "engine.h"
uint16_t ns_crc(const char *text, size_t size);
size_t ns_encode(const NS *e, uint32_t now, uint32_t tx_errors,
                 char *output, size_t capacity);
#endif
