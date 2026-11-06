/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *data;
    size_t used, capacity;
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
static void decimal(Writer *w, uint32_t n)
{
    char digits[10];
    unsigned count = 0;
    do {
        digits[count++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
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
    character(&w, 'K');
    character(&w, 'E');
    character(&w, '1');
    decimal(&w, e->sequence);
    decimal(&w, e->now);
    decimal(&w, e->state);
    decimal(&w, e->result);
    decimal(&w, e->fault);
    decimal(&w, e->raw);
    decimal(&w, e->qualified);
    decimal(&w, e->valid);
    decimal(&w, e->opened);
    decimal(&w, e->starts);
    decimal(&w, e->completed);
    decimal(&w, e->canceled);
    decimal(&w, e->aborted);
    decimal(&w, e->age);
    decimal(&w, e->last_ms);
    decimal(&w, e->baseline);
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
