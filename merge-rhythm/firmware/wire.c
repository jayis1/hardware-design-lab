/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void character(Writer *w, char c)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
}

static void literal(Writer *w, const char *text)
{
    while (*text)
        character(w, *text++);
}

static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned n = 0;
    do {
        digits[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (n)
        character(w, digits[--n]);
}

uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i=0; i<length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned j=0; j<8; ++j) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* Fixed policy version in every snapshot supports late attachment. No boot
 * message clears retained evidence. All counters are decimal unsigned. */
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!e || !out || !engine_valid(e))
        return 0;
    Writer w = {out, capacity, 0, false};
    literal(&w, "MR1");
    const uint32_t fields[] = {
        e->now, e->state, e->mask, e->known, e->pending,
        e->admitted, e->left, e->right, e->abandoned, e->rejected,
        e->alternations, e->longest, e->streak, e->latency_sum, e->latency_max,
        e->segments, e->ambiguous, e->orphans, e->full, e->timeouts,
        e->gaps, e->tx, e->fault
    };
    for (unsigned i=0; i<sizeof fields/sizeof fields[0]; ++i) {
        character(&w, ',');
        decimal(&w, fields[i]);
    }
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    character(&w, '*');
    const char hex[] = "0123456789ABCDEF";
    for (int shift=12; shift>=0; shift-=4)
        character(&w, hex[(crc >> shift) & 15u]);
    character(&w, '\n');
    return w.failed ? 0 : w.used;
}
