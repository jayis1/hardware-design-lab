/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}
typedef struct { char *out; size_t size, used; bool ok; } Writer;
static void put(Writer *w, char c)
{
    if (w->used >= w->size) {
        w->ok = false;
        return;
    }
    w->out[w->used++] = c;
}
static void field(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned n = 0;
    do {
        digits[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    put(w, ',');
    while (n)
        put(w, digits[--n]);
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || !engine_valid(e))
        return 0;
    Writer w = {out, capacity, 0, true};
    put(&w, 'C');
    put(&w, 'T');
    put(&w, '1');
    field(&w, e->now);
    field(&w, e->mask);
    field(&w, e->known);
    field(&w, e->gaps);
    field(&w, e->tx);
    field(&w, e->fault);
    field(&w, e->halted);
    for (unsigned i = 0; i < 2; ++i) {
        const Holder *h = &e->holder[i];
        field(&w, h->state);
        field(&w, h->origin);
        field(&w, h->elapsed);
        field(&w, h->started);
        field(&w, h->delivered);
        field(&w, h->returned);
        field(&w, h->abandoned);
        field(&w, h->sum_ms);
        field(&w, h->max_ms);
        field(&w, h->last_ms);
        field(&w, h->last_kind);
        field(&w, h->ambiguities);
        field(&w, h->timeouts);
    }
    if (!w.ok)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    const char *hex = "0123456789ABCDEF";
    put(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15u]);
    put(&w, '\n');
    return w.ok ? w.used : 0;
}
