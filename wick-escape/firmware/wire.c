/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *out;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void put(Writer *w,char ch)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = ch;
}

static void number(Writer *w,uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    put(w, ',');
    while (count)
        put(w, digits[--count]);
}

uint16_t wire_crc(const char *data,size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned j = 0; j < 8; ++j) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

size_t wire_encode(const Engine *e,uint32_t sequence,char *out,size_t capacity)
{
    if (!out || !engine_valid(e))
        return 0;
    Writer w = {out, capacity, 0, false};
    put(&w, 'W');
    put(&w, '1');
    number(&w, sequence);
    number(&w, e->now);
    number(&w, e->state);
    number(&w, e->wet);
    number(&w, e->faults);
    number(&w, e->elapsed);
    for (unsigned i = 0; i < CHANNELS; ++i)
        number(&w, e->adc[i]);
    for (unsigned i = 0; i < CHANNELS; ++i)
        number(&w, e->first[i]);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    put(&w, ',');
    static const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15u]);
    put(&w, '\n');
    return w.failed ? 0 : w.used;
}
