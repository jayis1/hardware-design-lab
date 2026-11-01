/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc=65535u;
    for (size_t i=0;i<length;++i) {
        crc ^= (uint16_t)((uint8_t)data[i]<<8);
        for (unsigned bit=0;bit<8;++bit)
            crc=(uint16_t)((crc<<1)^((crc & 0x8000u)?0x1021u:0u));
    }
    return crc;
}
typedef struct {
    char *out;
    size_t used, capacity;
    bool failed;
} Writer;
static void byte(Writer *w, char value)
{
    if (w->used>=w->capacity) {
        w->failed=true;
        return;
    }
    w->out[w->used++]=value;
}
static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned count=0;
    do {
        digits[count++]=(char)('0'+value%10u);
        value/=10u;
    } while (value);
    byte(w,',');
    while (count)
        byte(w,digits[--count]);
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || !engine_valid(e))
        return 0;
    Writer w={out,0,capacity,false};
    byte(&w,'B');
    byte(&w,'T');
    byte(&w,'1');
    const uint32_t values[]={e->now,e->state,e->levels,e->valid,e->starts,
        e->accepted,e->rejected,e->aborted,e->gaps,e->faults,e->tx_errors,
        e->age,e->tainted,e->selected,e->outcome,e->depth,e->cancelled,
        e->permutation,e->positive,e->negative,
        e->packed[0],e->packed[1],e->packed[2],e->packed[3]};
    for (unsigned i=0;i<sizeof values/sizeof values[0];++i)
        decimal(&w,values[i]);
    if (w.failed)
        return 0;
    uint16_t crc=wire_crc(out,w.used);
    byte(&w,'*');
    for (int shift=12;shift>=0;shift-=4)
        byte(&w,"0123456789ABCDEF"[(crc>>shift)&15u]);
    byte(&w,'\n');
    size_t length=w.used;
    byte(&w,'\0');
    return w.failed?0:length;
}
