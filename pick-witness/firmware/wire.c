/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *out;
    size_t capacity;
    size_t length;
    bool failed;
} Writer;
static void character(Writer *w, char c)
{
    if (w->length>=w->capacity) {
        w->failed=true;
        return;
    }
    w->out[w->length++]=c;
}
static void text(Writer *w, const char *s)
{
    while (*s)
        character(w,*s++);
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned n=0;
    do {
        digits[n++]=(char)('0'+value%10u);
        value/=10u;
    } while (value);
    while (n)
        character(w,digits[--n]);
}
static void field(Writer *w, uint32_t value)
{
    character(w,',');
    number(w,value);
}
uint16_t wire_crc(const char *bytes, size_t length)
{
    uint16_t crc=0xffffu;
    for (size_t i=0;i<length;++i) {
        crc^=(uint16_t)(uint8_t)bytes[i]<<8;
        for (unsigned bit=0;bit<8;++bit)
            crc=(uint16_t)((crc<<1)^((crc&0x8000u)?0x1021u:0u));
    }
    return crc;
}
size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || capacity<2)
        return 0;
    Writer w={out,capacity,0,false};
    text(&w,"@PW1");
    field(&w,sequence);
    field(&w,e->now);
    field(&w,e->stable);
    field(&w,e->ready);
    field(&w,e->pending);
    field(&w,e->accepted);
    field(&w,e->rejected);
    field(&w,e->anomalies);
    field(&w,e->gaps);
    field(&w,e->events);
    field(&w,e->reason);
    field(&w,e->last_interval);
    field(&w,e->min_interval==UINT32_MAX?0:e->min_interval);
    field(&w,e->max_interval);
    field(&w,e->transport_losses);
    field(&w,e->saturated);
    field(&w,QUALIFY_MS);
    field(&w,MAX_BEAT_MS);
    if (w.failed)
        return 0;
    uint16_t crc=wire_crc(out,w.length);
    character(&w,'*');
    const char *hex="0123456789ABCDEF";
    for (int shift=12;shift>=0;shift-=4)
        character(&w,hex[(crc>>shift)&15u]);
    character(&w,'\n');
    if (w.failed)
        return 0;
    return w.length;
}
