/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* Fixed-width hexadecimal avoids libc, allocation and locale dependence.
 * CRC-16/CCITT-FALSE covers the payload, including commas, before '*'. */
static uint16_t crc_byte(uint16_t crc, uint8_t byte)
{
    crc ^= (uint16_t)byte << 8;
    for (unsigned i = 0; i < 8; ++i)
        crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    return crc;
}
static char hex(unsigned n)
{
    return "0123456789ABCDEF"[n & 15u];
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!engine_valid(e))
        return 0;
    uint32_t fields[] = {
        e->now, e->state, e->mask, e->known, e->depth,
        e->stack[0], e->stack[1], e->stack[2], e->stack[3],
        e->started, e->completed, e->aborted, e->pushes, e->pops,
        e->abandoned, e->order_errors, e->collisions, e->gaps, e->tx, e->fault
    };
    size_t count = sizeof fields / sizeof fields[0];
    size_t length = 3u + count * 9u + 6u;
    if (capacity <= length)
        return 0;
    size_t p = 0;
    out[p++] = 'T';
    out[p++] = 'S';
    out[p++] = '1';
    for (size_t i = 0; i < count; ++i) {
        out[p++] = ',';
        for (unsigned shift = 32; shift; shift -= 4)
            out[p++] = hex(fields[i] >> (shift - 4u));
    }
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < p; ++i)
        crc = crc_byte(crc, (uint8_t)out[i]);
    out[p++] = '*';
    for (unsigned shift = 16; shift; shift -= 4)
        out[p++] = hex(crc >> (shift - 4u));
    out[p++] = '\n';
    out[p] = '\0';
    return p;
}
