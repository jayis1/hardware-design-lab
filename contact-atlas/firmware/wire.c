/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* Fixed-capacity ASCII serialization; failure never emits a partial frame. */
typedef struct {
    char *out;
    size_t used;
    size_t capacity;
    bool failed;
} Writer;

static void character(Writer *w, char value)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = value;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        character(w, *s++);
}

static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        character(w, digits[--count]);
}

static void field(Writer *w, uint32_t value)
{
    character(w, ',');
    decimal(w, value);
}

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static size_t finish(Writer *w)
{
    if (w->failed)
        return 0;
    uint16_t crc = wire_crc(w->out, w->used);
    const char *hex = "0123456789ABCDEF";
    character(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(w, hex[(crc >> shift) & 15]);
    character(w, '\n');
    return w->failed ? 0 : w->used;
}

size_t wire_status(char *out, size_t capacity, const Atlas *a,
                   uint32_t now, uint32_t sequence, uint32_t loss)
{
    if (!out || !a || !capacity)
        return 0;
    Writer w = {out, 0, capacity, false};
    text(&w, "CA1,S");
    field(&w, sequence);
    field(&w, now);
    field(&w, a->known);
    field(&w, a->travelling);
    field(&w, a->raw);
    field(&w, a->stable);
    field(&w, a->transitions);
    field(&w, a->gaps);
    field(&w, a->invalid);
    field(&w, a->dropped);
    field(&w, loss);
    field(&w, a->candidate_age);
    return finish(&w);
}

size_t wire_event(char *out, size_t capacity, const AtlasEvent *e,
                  uint32_t sequence)
{
    if (!out || !e || !capacity)
        return 0;
    Writer w = {out, 0, capacity, false};
    text(&w, "CA1,E");
    field(&w, sequence);
    field(&w, e->time);
    field(&w, e->number);
    field(&w, e->from);
    field(&w, e->to);
    field(&w, e->duration);
    field(&w, e->zero_ms);
    field(&w, e->multi_ms);
    field(&w, e->changes);
    return finish(&w);
}
