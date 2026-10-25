/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void character(Writer *w, char c)
{
    if (w->used>=w->capacity) {
        w->failed=true;
        return;
    }
    w->data[w->used++]=c;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        character(w,*s++);
}

static void number(Writer *w, uint32_t n)
{
    char digits[10];
    size_t count=0;
    do {
        digits[count++]=(char)('0'+n%10u);
        n/=10u;
    } while (n);
    while (count)
        character(w,digits[--count]);
}

static void field(Writer *w, uint32_t n)
{
    character(w,',');
    number(w,n);
}

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc=0xffffu;
    for (size_t i=0; i<length; ++i) {
        crc^=(uint16_t)((uint16_t)(uint8_t)data[i]<<8);
        for (unsigned bit=0; bit<8; ++bit) {
            if (crc & 0x8000u)
                crc=(uint16_t)((crc<<1)^0x1021u);
            else
                crc=(uint16_t)(crc<<1);
        }
    }
    return crc;
}

static void hexadecimal(Writer *w, uint16_t n)
{
    const char *digits="0123456789ABCDEF";
    character(w,digits[(n>>12)&15u]);
    character(w,digits[(n>>8)&15u]);
    character(w,digits[(n>>4)&15u]);
    character(w,digits[n&15u]);
}

size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || !capacity)
        return 0;
    Writer w={out,capacity,0,false};
    text(&w,"SE1");
    field(&w,sequence);
    field(&w,e->now);
    field(&w,engine_flags(e));
    field(&w,e->levels);
    field(&w,e->direction);
    field(&w,e->crossings);
    field(&w,e->period_ms);
    field(&w,e->east_ms);
    field(&w,e->west_ms);
    field(&w,e->left_seen);
    field(&w,e->right_seen);
    field(&w,e->left_missed);
    field(&w,e->right_missed);
    field(&w,e->faults);
    field(&w,e->gaps);
    field(&w,e->timeouts);
    field(&w,e->transport_losses);
    if (w.failed)
        return 0;
    uint16_t crc=wire_crc(w.data,w.used);
    character(&w,'*');
    hexadecimal(&w,crc);
    character(&w,'\n');
    if (w.failed)
        return 0;
    return w.used;
}
