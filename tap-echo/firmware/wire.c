/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>
typedef struct {
    char *data;
    size_t capacity;
    size_t length;
    bool failed;
} Writer;
static void byte(Writer *w, char c)
{
    if (w->length >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->length++] = c;
}
static void decimal(Writer *w, uint32_t value)
{
    char reverse[10];
    size_t count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    byte(w, ',');
    while (count)
        byte(w, reverse[--count]);
}
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0));
    }
    return crc;
}
size_t wire_encode(const Engine *e, char *output, size_t capacity)
{
    if (!output || !engine_valid(e))
        return 0;
    Writer w = {output, capacity, 0, false};
    byte(&w, 'T');
    byte(&w, 'E');
    byte(&w, '1');
    decimal(&w, e->now);
    decimal(&w, e->samples);
    decimal(&w, e->missed);
    decimal(&w, e->tx_errors);
    decimal(&w, e->fault);
    decimal(&w, e->raw);
    decimal(&w, e->qualified);
    decimal(&w, e->valid);
    decimal(&w, e->armed);
    decimal(&w, e->active);
    decimal(&w, e->recording);
    decimal(&w, e->references);
    decimal(&w, e->trials);
    decimal(&w, e->matches);
    decimal(&w, e->aborted);
    decimal(&w, e->timeouts);
    decimal(&w, e->reference_count);
    decimal(&w, e->count);
    decimal(&w, e->result);
    decimal(&w, e->max_error);
    decimal(&w, e->wrong);
    decimal(&w, e->age);
    for (unsigned i = 0; i < CAPACITY; ++i) {
        decimal(&w, e->reference[i].pad);
        decimal(&w, e->reference[i].interval);
    }
    for (unsigned i = 0; i < CAPACITY; ++i) {
        decimal(&w, e->events[i].pad);
        decimal(&w, e->events[i].interval);
    }
    if (w.failed)
        return 0;
    const uint16_t crc = wire_crc(output, w.length);
    const char hex[] = "0123456789ABCDEF";
    byte(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15]);
    byte(&w, '\n');
    return w.failed ? 0 : w.length;
}
