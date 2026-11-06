/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>

typedef struct {
    char *data;
    size_t used;
    size_t capacity;
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

static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    size_t count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(w, ',');
    while (count)
        character(w, digits[--count]);
}

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
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

static void array(Writer *w, const uint32_t values[4])
{
    for (unsigned i = 0; i < 4; ++i)
        decimal(w, values[i]);
}

size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!buffer || !engine_valid(e))
        return 0;
    Writer w = {buffer, 0, capacity, false};
    character(&w, 'A');
    character(&w, 'R');
    character(&w, '1');
    /* AR1 identifies a fixed cohort-rank contract on every snapshot. */
    decimal(&w, e->sequence);
    decimal(&w, e->now);
    decimal(&w, e->state);
    decimal(&w, e->fault);
    decimal(&w, e->raw);
    decimal(&w, e->qualified);
    decimal(&w, e->valid);
    decimal(&w, e->epoch);
    decimal(&w, e->occupied);
    decimal(&w, e->insertions);
    decimal(&w, e->removals);
    decimal(&w, e->abandoned);
    decimal(&w, e->ambiguous);
    decimal(&w, e->last_mask);
    array(&w, e->rank);
    array(&w, e->stamp);
    array(&w, e->low);
    array(&w, e->high);
    decimal(&w, e->tx_errors);
    decimal(&w, e->gaps);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(buffer, w.used);
    const char hex[] = "0123456789ABCDEF";
    character(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15u]);
    character(&w, '\n');
    return w.failed ? 0 : w.used;
}
