/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* Bounded formatter: no heap, printf implementation or floating point. */
typedef struct {
    char *out;
    size_t length;
    size_t capacity;
    bool failed;
} Writer;
static void put(Writer *w, char c)
{
    if (w->length + 1 >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->length++] = c;
}
static void decimal(Writer *w, uint32_t value)
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
static uint16_t checksum(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)((uint16_t)(uint8_t)data[i] << 8);
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
    if (!e || !out || capacity < 8 || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    put(&w, 'P');
    put(&w, 'F');
    put(&w, '1');
    decimal(&w, e->now);
    decimal(&w, e->state);
    decimal(&w, e->mask);
    decimal(&w, e->known);
    decimal(&w, e->gaps);
    decimal(&w, e->tx_errors);
    decimal(&w, e->faults);
    decimal(&w, e->result.id);
    decimal(&w, e->result.reason);
    decimal(&w, e->result.seen);
    for (unsigned i = 0; i < 4; ++i)
        decimal(&w, e->result.at[i]);
    decimal(&w, e->result.duration);
    if (w.failed)
        return 0;
    uint16_t crc = checksum(out, w.length);
    static const char hex[] = "0123456789ABCDEF";
    put(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15]);
    put(&w, '\n');
    if (w.failed)
        return 0;
    out[w.length] = 0;
    return w.length;
}
