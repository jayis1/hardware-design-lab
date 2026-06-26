/*
 * lz4.c — Embedded LZ4 compressor (BSD-licensed subset)
 * Author: jayis1 (this port; original LZ4 algorithm by Yann Collet, BSD-2-Clause)
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0 (this file); LZ4 algorithm is BSD-2-Clause
 *
 * Minimal LZ4 block-format compressor/decompressor for embedded use.
 * Optimized for the seismic waveform packets in Glaciator-Motes, which
 * are repetitive (quiet background → long runs of zeros) and compress
 * 3-5× with this implementation.
 */

#include "lz4.h"
#include <string.h>

#define LZ4_MIN_MATCH       4
#define LZ4_MAX_MATCH       255
#define LZ4_HASH_SIZE       16384   /* 16 K entries, 2-byte hash */
#define LZ4_SKIP_TRIGGER    6

/* ---- 32-bit read helper ------------------------------------------------- */
static uint32_t read32(const uint8_t *p)
{
    return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* ---- Hash 4 bytes into 14-bit table index ------------------------------- */
static uint16_t lz4_hash(uint32_t v)
{
    return (uint16_t)((v * 2654435761u) >> 18) & (LZ4_HASH_SIZE - 1);
}

/* ---- Encode a variable-length literal/match length ---------------------- */
static uint8_t *lz4_emit_len(uint8_t *dst, uint32_t len)
{
    while (len >= 255) {
        *dst++ = 255;
        len   -= 255;
    }
    *dst++ = (uint8_t)len;
    return dst;
}

/* ---- Compressor --------------------------------------------------------- */
int32_t lz4_compress(const uint8_t *src, uint8_t *dst,
                      uint32_t src_size, uint32_t dst_capacity)
{
    /* Simplified LZ4 block compressor using a hash table of last-seen positions. */
    static uint16_t hash_table[LZ4_HASH_SIZE];
    const uint8_t *src_end       = src + src_size;
    const uint8_t *src_p         = src;
    const uint8_t *src_anchor    = src;
    const uint8_t *src_mflimit   = src + src_size - 12;  /* minimum useful */
    uint8_t       *dst_p         = dst;
    uint8_t       *dst_end       = dst + dst_capacity;
    const uint8_t *match;
    uint32_t      match_len;
    uint32_t      literal_len;

    memset(hash_table, 0, sizeof(hash_table));

    if (src_size < 13) {
        /* Too small to compress: emit a single literal block. */
        if (dst_p + 1 + src_size > dst_end) return -1;
        dst_p = lz4_emit_len(dst_p, src_size);
        memcpy(dst_p, src, src_size);
        dst_p += src_size;
        return (int32_t)(dst_p - dst);
    }

    while (src_p < src_mflimit) {
        uint32_t v = read32(src_p);
        uint16_t h = lz4_hash(v);
        uint16_t prev = hash_table[h];
        hash_table[h] = (uint16_t)(src_p - src);

        /* Check if prev points to a real match within 64 KB window */
        if (prev != 0 && src_p - src > prev &&
            src_p - (src + prev) < 65536 &&
            read32(src + prev) == v) {

            match = src + prev;
            /* Extend match forward */
            match_len = LZ4_MIN_MATCH;
            while (src_p + match_len < src_end &&
                   match + match_len < src_p &&
                   src_p[match_len] == match[match_len] &&
                   match_len < LZ4_MAX_MATCH + LZ4_MIN_MATCH) {
                match_len++;
            }

            /* Emit literals since anchor */
            literal_len = (uint32_t)(src_p - src_anchor);
            if (dst_p + literal_len + 8 > dst_end) return -1;

            uint8_t token = (literal_len < 15) ?
                            (uint8_t)(literal_len << 4) : (uint8_t)(15 << 4);
            uint32_t ml = match_len - LZ4_MIN_MATCH;
            token |= (ml < 15) ? (uint8_t)ml : 15;
            *dst_p++ = token;
            if (literal_len >= 15) dst_p = lz4_emit_len(dst_p, literal_len - 15);
            memcpy(dst_p, src_anchor, literal_len);
            dst_p += literal_len;

            /* Emit match offset (16-bit little-endian) */
            uint16_t offset = (uint16_t)(src_p - match);
            *dst_p++ = (uint8_t)(offset & 0xFF);
            *dst_p++ = (uint8_t)(offset >> 8);

            /* Emit extra match length if > 15 */
            if (ml >= 15) dst_p = lz4_emit_len(dst_p, ml - 15);

            src_p       += match_len;
            src_anchor  = src_p;
        } else {
            src_p++;
        }
    }

    /* Emit remaining literals */
    literal_len = (uint32_t)(src_end - src_anchor);
    if (dst_p + literal_len + 1 > dst_end) return -1;
    uint8_t token = (literal_len < 15) ?
                    (uint8_t)(literal_len << 4) : (uint8_t)(15 << 4);
    *dst_p++ = token;
    if (literal_len >= 15) dst_p = lz4_emit_len(dst_p, literal_len - 15);
    memcpy(dst_p, src_anchor, literal_len);
    dst_p += literal_len;

    return (int32_t)(dst_p - dst);
}

/* ---- Decompressor ------------------------------------------------------- */
int32_t lz4_decompress(const uint8_t *src, uint8_t *dst,
                        uint32_t src_size, uint32_t dst_max)
{
    const uint8_t *src_p   = src;
    const uint8_t *src_end = src + src_size;
    uint8_t       *dst_p   = dst;
    uint8_t       *dst_end = dst + dst_max;

    while (src_p < src_end) {
        uint8_t token = *src_p++;
        uint32_t literal_len = token >> 4;
        uint32_t match_len   = token & 0x0F;

        /* Extended literal length */
        if (literal_len == 15) {
            uint8_t b;
            do {
                if (src_p >= src_end) return -1;
                b = *src_p++;
                literal_len += b;
            } while (b == 255);
        }

        /* Copy literals */
        if (src_p + literal_len > src_end) return -1;
        if (dst_p + literal_len > dst_end) return -1;
        memcpy(dst_p, src_p, literal_len);
        dst_p  += literal_len;
        src_p  += literal_len;

        if (src_p >= src_end) break;   /* last block: literals only */

        /* Read match offset */
        if (src_p + 2 > src_end) return -1;
        uint16_t offset = (uint16_t)src_p[0] | ((uint16_t)src_p[1] << 8);
        src_p += 2;
        if (offset == 0) return -1;

        /* Extended match length */
        if (match_len == 15) {
            uint8_t b;
            do {
                if (src_p >= src_end) return -1;
                b = *src_p++;
                match_len += b;
            } while (b == 255);
        }
        match_len += LZ4_MIN_MATCH;

        /* Copy match (may overlap) */
        if (dst_p + match_len > dst_end) return -1;
        uint8_t *match = dst_p - offset;
        if (match < dst) return -1;
        for (uint32_t i = 0; i < match_len; i++) {
            dst_p[i] = match[i];
        }
        dst_p += match_len;
    }

    return (int32_t)(dst_p - dst);
}