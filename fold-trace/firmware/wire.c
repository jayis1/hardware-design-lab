/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* Decimal, bounded, allocation-free telemetry. No printf/newlib dependency. */
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

size_t wire_status(char *out, size_t capacity, const Engine *e,
                   uint32_t now, uint32_t sequence, uint32_t transport_loss)
{
    if (!out || !e || !capacity)
        return 0;
    Writer w = {out, 0, capacity, false};
    text(&w, "FT1");
    field(&w, sequence);
    field(&w, now);
    field(&w, (uint32_t)e->state);
    field(&w, (uint32_t)e->reason);
    field(&w, e->step);
    field(&w, e->mask);
    field(&w, e->good);
    field(&w, e->bad);
    field(&w, e->gaps);
    field(&w, e->duration);
    for (unsigned i = 0; i < STATIONS; ++i)
        field(&w, e->dwell[i]);
    field(&w, transport_loss);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    const char *hex = "0123456789ABCDEF";
    character(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    if (w.failed)
        return 0;
    return w.used;
}
