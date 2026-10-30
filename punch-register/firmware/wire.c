/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

/* CRC-16/CCITT-FALSE: poly 0x1021, init 0xffff, no reflection/xorout. */
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned b = 0; b < 8; ++b) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

typedef struct {
    char *data;
    size_t used;
    size_t capacity;
    bool failed;
} Writer;

static void byte(Writer *w, char c)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
}

static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned length = 0;
    do {
        reverse[length++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (length)
        byte(w, reverse[--length]);
}

static void field(Writer *w, uint32_t value)
{
    byte(w, ',');
    number(w, value);
}

size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || !capacity || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    const char *marker = "PR1";
    while (*marker)
        byte(&w, *marker++);
    field(&w, e->now);
    field(&w, e->raw);
    field(&w, e->mask);
    field(&w, e->qualified);
    field(&w, e->state);
    field(&w, e->fault);
    field(&w, e->starts);
    field(&w, e->good);
    field(&w, e->bad);
    field(&w, e->aborted);
    field(&w, e->extra);
    field(&w, e->gaps);
    field(&w, e->tx_errors);
    field(&w, e->reason);
    field(&w, e->elapsed);
    field(&w, e->bottoms);
    field(&w, e->last_reason);
    field(&w, e->last_duration);
    field(&w, e->seat_ms);
    field(&w, e->empty_ms);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    byte(&w, '*');
    for (unsigned shift = 16; shift; shift -= 4)
        byte(&w, "0123456789ABCDEF"[(crc >> (shift - 4)) & 15u]);
    byte(&w, '\n');
    size_t length = w.used;
    byte(&w, '\0');
    return w.failed ? 0 : length;
}
