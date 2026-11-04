/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* No printf, allocation or locale-sensitive conversion on the MCU. */
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
    byte(&w, 'D', true);
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
    number(&w, e->full);
    number(&w, e->partial);
    number(&w, e->disturbed);
    number(&w, e->aborted);
    number(&w, e->outcome);
    number(&w, e->age);
    number(&w, e->seen);
    number(&w, e->rebound);
    number(&w, e->baseline);
    number(&w, e->quiet);
    for (unsigned i = 0; i < 4; ++i)
        number(&w, e->offset[i]);
    number(&w, e->tx_errors);
    uint16_t crc = w.crc;
    const char hex[] = "0123456789ABCDEF";
    byte(&w, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15u], false);
    byte(&w, '\n', false);
    /* The returned count includes LF; no terminating NUL is transmitted. */
    if (w.failed)
        return 0;
    return w.size;
}
