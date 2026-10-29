/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>

typedef struct {
    char *out;
    size_t used;
    size_t capacity;
    bool failed;
} Writer;

static void byte(Writer *w, char value)
{
    if (w->used + 1 >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = value;
}

static void decimal(Writer *w, uint32_t value)
{
    char reversed[10];
    size_t count = 0;
    do {
        reversed[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        byte(w, reversed[--count]);
}

static uint16_t crc16(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || capacity < 8 || !engine_valid(e))
        return 0;
    Writer w = { out, 0, capacity, false };
    byte(&w, 'R');
    byte(&w, 'A');
    byte(&w, '1');
    const uint32_t values[] = {
        e->now, (uint32_t)e->phase, e->mask, e->starts,
        e->completed, e->aborted, e->duration, e->forward,
        e->reverse, e->seated_reverse, e->rollback,
        e->faults, e->gaps, e->tx_errors
    };
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        byte(&w, ',');
        decimal(&w, values[i]);
    }
    if (w.failed)
        return 0;
    uint16_t crc = crc16(out, w.used);
    byte(&w, '*');
    static const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15u]);
    byte(&w, '\n');
    if (w.failed)
        return 0;
    out[w.used] = 0;
    return w.used;
}
