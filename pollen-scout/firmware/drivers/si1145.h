/*
 * si1145.h - SI1145 UV index driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef SI1145_H
#define SI1145_H

int   si1145_init(void);
float si1145_read_uv(void);   /* returns UV index 0..14 */

#endif /* SI1145_H */