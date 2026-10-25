/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *buffer;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void character(Writer *w, char c)
{
    if (w->failed)
        return;
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->buffer[w->used++] = c;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        character(w, *s++);
}

static void number(Writer *w, uint64_t value)
{
    char digits[20];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (count)
        character(w, digits[--count]);
}

static void field(Writer *w, uint64_t value)
{
    character(w, ',');
    number(w, value);
}

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000)
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void checksum(Writer *w)
{
    static const char hex[] = "0123456789ABCDEF";
    if (w->failed)
        return;
    uint16_t crc = wire_crc(w->buffer, w->used);
    character(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(w, hex[(crc >> shift) & 15]);
    character(w, '\n');
}

static void route_fields(Writer *w, const Route *r)
{
    field(w, r->count);
    field(w, r->minimum);
    field(w, r->maximum);
    field(w, r->sum);
    for (unsigned i = 0; i < HISTOGRAM_BINS; ++i)
        field(w, r->bins[i]);
}

size_t wire_encode(const Engine *e, uint32_t sequence, uint32_t errors,
                   char *buffer, size_t capacity)
{
    if (!e || !buffer || capacity < 8)
        return 0;
    Writer w = {buffer, capacity, 0, false};
    text(&w, "RTC1");
    field(&w, sequence);
    field(&w, e->time);
    field(&w, e->state);
    field(&w, e->levels);
    field(&w, e->entries);
    field(&w, e->accepted);
    field(&w, e->rejected);
    field(&w, e->orphans);
    field(&w, e->gaps);
    field(&w, e->block_events);
    field(&w, e->last_route);
    field(&w, e->last_ms);
    field(&w, e->reason);
    field(&w, engine_flags(e));
    field(&w, errors);
    for (unsigned i = 0; i < 3; ++i)
        route_fields(&w, &e->route[i]);
    checksum(&w);
    return w.failed ? 0 : w.used;
}
