/*
 * drivers/sha256.h — SHA-256 interface
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef REBARSCOPE_SHA256_H
#define REBARSCOPE_SHA256_H

#include <stdint.h>

typedef struct {
    uint32_t h[8];
    uint8_t  buf[64];
    uint32_t hlen;
    uint32_t total;
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const uint8_t *data, uint32_t len);
void sha256_final(sha256_ctx_t *ctx, uint8_t out[32]);

#endif /* REBARSCOPE_SHA256_H */