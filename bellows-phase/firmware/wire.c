/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool valid;
} Writer;

static void character(Writer *w, char value)
{
    if (w->used >= w->capacity) {
        w->valid = false;
        return;
    }
    w->data[w->used++] = value;
}

static void number(Writer *w, uint32_t value)
{
    char digits[10];
    size_t length = 0;
    do {
        digits[length++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    character(w, ',');
    while (length)
        character(w, digits[--length]);
}

size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!e || !out || !engine_valid(e))
        return 0;
    Writer w = {out, capacity, 0, true};
    character(&w, 'B');
    character(&w, 'P');
    character(&w, '1');
    /* BP1 defines fixed policy and field order; no boot packet dependency. */
    number(&w, e->time);
    number(&w, e->raw);
    number(&w, e->levels);
    number(&w, e->known);
    number(&w, e->state);
    number(&w, e->anchor);
    number(&w, e->starts);
    number(&w, e->completed[0]);
    number(&w, e->completed[1]);
    number(&w, e->aborted);
    number(&w, e->active_ms);
    for (unsigned i = 0; i < 8; ++i)
        number(&w, e->bins[i]);
    number(&w, e->last_ms);
    number(&w, e->last_dir);
    number(&w, e->flags);
    number(&w, e->gaps);
    number(&w, e->tx);
    if (!w.valid)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    const char hex[] = "0123456789ABCDEF";
    character(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    character(&w, '\0');
    return w.valid ? w.used - 1 : 0;
}
