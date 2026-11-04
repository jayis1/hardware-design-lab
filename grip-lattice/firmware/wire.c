/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* Bounded decimal protocol; no formatted-I/O library or heap on target. */
typedef struct {
    char *data;
    size_t size, capacity;
    uint16_t crc;
    bool failed;
} Writer;
static void byte(Writer *w, char value, bool checksum)
{
    if (w->size >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->size++] = value;
    if (!checksum)
        return;
    w->crc ^= (uint16_t)(uint8_t)value << 8;
    for (unsigned i = 0; i < 8; ++i) {
        if (w->crc & 0x8000u)
            w->crc = (uint16_t)((w->crc << 1) ^ 0x1021u);
        else
            w->crc = (uint16_t)(w->crc << 1);
    }
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    byte(w, ',', true);
    while (count)
        byte(w, digits[--count], true);
}
size_t wire_encode(const Engine *e, char *destination, size_t capacity)
{
    if (!destination || !capacity || !engine_valid(e))
        return 0;
    Writer w = {destination, 0, capacity, 0xffffu, false};
    byte(&w, 'G', true);
    byte(&w, 'L', true);
    byte(&w, '1', true);
    number(&w, e->now);
    number(&w, e->samples);
    number(&w, e->phase);
    number(&w, e->fault);
    number(&w, e->raw);
    number(&w, e->mask);
    number(&w, e->valid);
    number(&w, e->starts);
    number(&w, e->completed);
    number(&w, e->aborted);
    number(&w, e->age);
    number(&w, e->unknown);
    number(&w, e->baseline);
    number(&w, e->tx_errors);
    for (unsigned i = 0; i < 8; ++i)
        number(&w, e->hist[i]);
    for (unsigned i = 0; i < 8; ++i)
        number(&w, e->runs[i]);
    for (unsigned i = 0; i < 8; ++i)
        number(&w, e->longest[i]);
    uint16_t crc = w.crc;
    const char hex[] = "0123456789ABCDEF";
    byte(&w, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15u], false);
    byte(&w, '\n', false);
    if (w.failed)
        return 0;
    return w.size;
}
