/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool valid;
} Writer;

static void put(Writer *w, char ch)
{
    if (!w->valid)
        return;
    if (w->used >= w->capacity) {
        w->valid = false;
        return;
    }
    w->data[w->used++] = ch;
}

static void literal(Writer *w, const char *s)
{
    while (*s)
        put(w, *s++);
}

static void decimal(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        put(w, reverse[--count]);
}

static void field(Writer *w, uint32_t value)
{
    put(w, ',');
    decimal(w, value);
}

uint16_t cw_crc(const char *text, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)((uint16_t)(uint8_t)text[i] << 8);
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

/* Fixed version carries fixed policy; there is no negotiation or mutable
 * configuration. Each frame repeats the most recent result for late join. */
size_t cw_encode(const CW *e, uint32_t now, uint32_t tx_errors,
                 char *output, size_t capacity)
{
    if (!output || !e || !capacity)
        return 0;
    Writer w = {output, capacity, 0, true};
    literal(&w, "CW1");
    field(&w, now);
    field(&w, e->state);
    field(&w, e->mask);
    field(&w, e->gaps);
    field(&w, tx_errors);
    field(&w, e->result.id);
    field(&w, e->result.reason);
    field(&w, e->result.keys);
    field(&w, e->result.spread);
    field(&w, e->result.overlap);
    field(&w, e->result.release);
    field(&w, e->result.duration);
    if (!w.valid)
        return 0;
    uint16_t crc = cw_crc(output, w.used);
    put(&w, '*');
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, digits[(crc >> shift) & 15]);
    put(&w, '\n');
    if (!w.valid)
        return 0;
    return w.used;
}
