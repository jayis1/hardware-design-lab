/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *out;
    size_t length;
    size_t capacity;
    bool failed;
} Writer;

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

static void character(Writer *w, char c)
{
    if (w->length + 1u >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->length++] = c;
}

static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    size_t count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(w, ',');
    while (count) {
        character(w, reverse[--count]);
    }
}

size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || capacity < 8u) {
        return 0;
    }
    Writer w = {out, 0, capacity, false};
    character(&w, 'C');
    character(&w, 'T');
    character(&w, '1');
    number(&w, sequence);
    number(&w, e->now);
    number(&w, (uint32_t)e->phase);
    number(&w, e->levels);
    number(&w, e->trial);
    number(&w, e->good);
    number(&w, e->bad);
    number(&w, e->gaps);
    number(&w, e->loss);
    number(&w, e->last.id);
    number(&w, e->last.ok ? 1u : 0u);
    number(&w, (uint32_t)e->last.reason);
    number(&w, e->last.start[0]);
    number(&w, e->last.start[1]);
    number(&w, e->last.finish[0]);
    number(&w, e->last.finish[1]);
    if (w.failed) {
        out[0] = 0;
        return 0;
    }
    uint16_t crc = wire_crc(out, w.length);
    character(&w, '*');
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4) {
        character(&w, hex[(crc >> shift) & 15u]);
    }
    character(&w, '\n');
    if (w.failed) {
        out[0] = 0;
        return 0;
    }
    out[w.length] = 0;
    return w.length;
}
