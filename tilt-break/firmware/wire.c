/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

uint16_t wire_crc(const char *data,size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned b = 0; b < 8; ++b)
            crc = (uint16_t)((crc & 0x8000u) ? ((uint32_t)crc << 1) ^ 0x1021u : (uint32_t)crc << 1);
    }
    return crc;
}

typedef struct {
    char *out;
    size_t used, capacity;
    bool failed;
} Writer;

static void put(Writer *w,char c)
{
    if (w->used + 1u >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = c;
}

static void number(Writer *w,int64_t value)
{
    char digits[20];
    size_t count = 0;
    put(w,',');
    if (value < 0) {
        put(w,'-');
        value = -value;
    }
    uint64_t n = (uint64_t)value;
    do {
        digits[count++] = (char)('0' + n % 10u);
        n /= 10u;
    } while (n && count < sizeof digits);
    while (count)
        put(w,digits[--count]);
}

size_t wire_encode(const Engine *e,uint32_t sequence,char *out,size_t capacity)
{
    if (!out || capacity < 8u || !engine_consistent(e))
        return 0;
    Writer w = {out,0,capacity,false};
    put(&w,'T');
    put(&w,'B');
    put(&w,'1');
    number(&w,sequence);
    number(&w,e->now);
    number(&w,e->state);
    number(&w,e->faults);
    number(&w,e->valid);
    for (unsigned i = 0; i < 4; ++i)
        number(&w,e->adc[i]);
    number(&w,e->baseline);
    number(&w,e->height);
    number(&w,e->elapsed);
    number(&w,e->have_release);
    number(&w,e->release_lo);
    number(&w,e->release_hi);
    number(&w,e->transit_ms);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out,w.used);
    static const char hex[] = "0123456789ABCDEF";
    put(&w,',');
    for (unsigned shift = 16; shift; shift -= 4)
        put(&w,hex[(crc >> (shift - 4)) & 15u]);
    put(&w,'\n');
    if (w.failed)
        return 0;
    out[w.used] = '\0';
    return w.used;
}
