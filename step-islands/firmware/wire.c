/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *data;
    size_t length, capacity;
    bool ok;
} Writer;
static void character(Writer *w, char c)
{
    if (w->length >= w->capacity) {
        w->ok = false;
        return;
    }
    w->data[w->length++] = c;
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    character(w, ',');
    while (count)
        character(w, digits[--count]);
}
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
size_t wire_encode(const Engine *e, char *output, size_t capacity)
{
    if (!output || !engine_valid(e))
        return 0;
    Writer w = {output, 0, capacity, true};
    character(&w, 'S');
    character(&w, 'I');
    character(&w, '1');
    number(&w, e->now);
    number(&w, e->samples);
    number(&w, e->gaps);
    number(&w, e->missed);
    number(&w, e->tx_errors);
    number(&w, e->raw);
    number(&w, e->hold);
    number(&w, e->credited);
    number(&w, e->seeded);
    number(&w, e->fault);
    number(&w, e->visits);
    number(&w, e->settings);
    number(&w, e->coverage);
    number(&w, e->qualified_ms);
    number(&w, e->unknown_ms);
    number(&w, engine_suggest(e));
    number(&w, engine_components(e));
    number(&w, engine_bridges(e));
    if (!w.ok)
        return 0;
    uint16_t crc = wire_crc(output, w.length);
    character(&w, '*');
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    size_t length = w.length;
    character(&w, '\0');
    return w.ok ? length : 0;
}
