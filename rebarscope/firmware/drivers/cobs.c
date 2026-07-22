/*
 * drivers/cobs.c — Consistent Overhead Byte Stuffing encoder/decoder
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "cobs.h"
#include <stdint.h>

uint8_t cobs_encode(const uint8_t *in, uint8_t len, uint8_t *out)
{
    uint8_t *code_ptr = out++;
    uint8_t code = 1;
    uint8_t out_idx = 1;

    for (uint8_t i = 0; i < len; i++) {
        if (in[i] == 0x00) {
            *code_ptr = code;
            code_ptr = &out[out_idx++];
            code = 1;
        } else {
            out[out_idx++] = in[i];
            code++;
            if (code == 0xFF) {
                *code_ptr = code;
                code_ptr = &out[out_idx++];
                code = 1;
            }
        }
    }
    *code_ptr = code;
    return out_idx;
}

uint8_t cobs_decode(const uint8_t *in, uint8_t len, uint8_t *out)
{
    uint8_t i = 0, out_idx = 0;
    while (i < len) {
        uint8_t code = in[i];
        if (i + code > len) return 0;   /* malformed */
        for (uint8_t j = 1; j < code; j++) {
            out[out_idx++] = in[i + j];
        }
        i += code;
        if (code < 0xFF && i < len) {
            out[out_idx++] = 0x00;
        }
    }
    return out_idx;
}