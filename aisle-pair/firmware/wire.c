/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

/* CRC-16/CCITT-FALSE detects damaged bytes, not forged measurements. */
uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0u));
    }
    return crc;
}

typedef struct {
    char *buffer;
    size_t used;
    size_t capacity;
    bool ok;
} Writer;

static void character(Writer *w, char value)
{
    if (w->used + 1u >= w->capacity) {
        w->ok = false;
        return;
    }
    w->buffer[w->used++] = value;
}

static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(w, ',');
    while (count)
        character(w, digits[--count]);
}

size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!buffer || !capacity || !engine_valid(e))
        return 0;
    Writer w = {buffer, 0, capacity, true};
    character(&w, 'A');
    character(&w, 'Y');
    character(&w, '1');
    const uint32_t fields[] = {
        e->sequence, e->now, e->phase, e->fault, e->raw, e->qualified,
        e->valid, e->intervals, e->unknown, e->invalid, e->episodes,
        e->run, e->maximum, e->gaps, e->tx_errors
    };
    for (unsigned i = 0; i < sizeof fields / sizeof fields[0]; ++i)
        decimal(&w, fields[i]);
    for (unsigned i = 0; i < CELLS; ++i)
        decimal(&w, e->bins[i]);
    if (!w.ok)
        return 0;
    uint16_t crc = wire_crc(buffer, w.used);
    character(&w, '*');
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15u]);
    character(&w, '\n');
    if (!w.ok)
        return 0;
    buffer[w.used] = 0;
    return w.used;
}
