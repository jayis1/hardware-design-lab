/*
 * lz4.h — Embedded LZ4 compressor (BSD-licensed subset)
 * Author: jayis1 (this port; original LZ4 by Yann Collet, BSD-2-Clause)
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0 (this file); LZ4 itself is BSD-2-Clause
 */

#ifndef LZ4_H
#define LZ4_H

#include <stdint.h>
#include <stddef.h>

/* Compress src[0..src_size) into dst, returning compressed size.
   dst_capacity must be >= src_size + a few bytes overhead. */
int32_t lz4_compress(const uint8_t *src, uint8_t *dst,
                      uint32_t src_size, uint32_t dst_capacity);

/* Decompress src[0..src_size) into dst[0..dst_max), returning output size. */
int32_t lz4_decompress(const uint8_t *src, uint8_t *dst,
                        uint32_t src_size, uint32_t dst_max);

#endif /* LZ4_H */