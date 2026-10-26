/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef CAM_WIRE_H
#define CAM_WIRE_H
#include <stddef.h>
#include "engine.h"
size_t cam_encode(const Cam *e, uint32_t now, uint32_t tx_errors,
                  char *output, size_t capacity);
#endif
