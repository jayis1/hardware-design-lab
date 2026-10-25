/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "wire.h"

typedef struct {
    char *out;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void put(Writer *w, char c)
{
    if (w->used+1>=w->capacity) {
        w->failed=true;
        return;
    }
    w->out[w->used++]=c;
}

static void number(Writer *w, uint32_t value)
{
    char digits[10];
    size_t count=0;
    do {
        digits[count++]=(char)('0'+value%10u);
        value/=10u;
    } while (value);
    put(w,',');
    while (count)
        put(w,digits[--count]);
}

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc=0xffffu;
    for (size_t i=0;i<length;++i) {
        crc^=(uint16_t)(uint8_t)data[i]<<8;
        for (unsigned bit=0;bit<8;++bit) {
            if (crc&0x8000u)
                crc=(uint16_t)((crc<<1)^0x1021u);
            else
                crc=(uint16_t)(crc<<1);
        }
    }
    return crc;
}

size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || capacity<2)
        return 0;
    Writer w={out,capacity,0,false};
    const char *prefix="$SW1";
    while (*prefix)
        put(&w,*prefix++);
    number(&w,sequence);
    number(&w,e->now);
    number(&w,(uint32_t)e->state);
    number(&w,(uint32_t)e->reason);
    number(&w,e->raw);
    number(&w,e->stable);
    number(&w,e->qualified?1u:0u);
    number(&w,e->cycle_ms);
    number(&w,e->residence_ms);
    number(&w,e->drain_ms);
    number(&w,e->longest_drain_ms);
    number(&w,e->rest_ms);
    number(&w,e->interruptions);
    number(&w,e->completed);
    number(&w,e->rejected);
    number(&w,e->gaps);
    number(&w,e->transport_losses);
    if (w.failed) {
        out[0]=0;
        return 0;
    }
    uint16_t crc=wire_crc(out,w.used);
    const char *hex="0123456789ABCDEF";
    put(&w,'*');
    for (int shift=12;shift>=0;shift-=4)
        put(&w,hex[(crc>>shift)&15u]);
    put(&w,'\n');
    if (w.failed) {
        out[0]=0;
        return 0;
    }
    out[w.used]=0;
    return w.used;
}
