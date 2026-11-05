/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *data;
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
    w->data[w->used++] = value;
}

static void decimal(Writer *w, uint32_t value)
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

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0));
    }
    return crc;
}

size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!buffer || !engine_valid(e))
        return 0;
    Writer w = {buffer, 0, capacity, false};
    character(&w, 'T');
    character(&w, 'H');
    character(&w, '1');
    decimal(&w, e->sequence);
    decimal(&w, e->now);
    decimal(&w, e->state);
    decimal(&w, e->fault);
    decimal(&w, e->raw);
    decimal(&w, e->qualified);
    decimal(&w, e->valid);
    decimal(&w, e->origin);
    decimal(&w, e->age);
    decimal(&w, e->baseline);
    decimal(&w, e->hold);
    decimal(&w, e->reached_ms);
    decimal(&w, e->released_ms);
    decimal(&w, e->finish_ms);
    decimal(&w, e->result);
    decimal(&w, e->tx_errors);
    decimal(&w, e->gaps);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(buffer, w.used);
    character(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, "0123456789ABCDEF"[(crc >> shift) & 15]);
    character(&w, '\n');
    return w.failed ? 0 : w.used;
}
