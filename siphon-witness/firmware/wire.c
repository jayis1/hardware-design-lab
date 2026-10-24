/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *data;
    size_t capacity;
    size_t used;
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
static void text(Writer *w, const char *value)
{
    while (*value)
        character(w, *value++);
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    size_t count = 0;
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
    number(w, value);
}
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffff;
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
size_t wire_encode(const Engine *e, uint32_t now, uint32_t sequence,
                   char *output, size_t capacity)
{
    if (!e || !output || !capacity)
        return 0;
    Writer w = {output, capacity, 0, false};
    text(&w, "SW1");
    field(&w, sequence);
    field(&w, now);
    field(&w, e->trial);
    field(&w, (uint32_t)e->phase);
    field(&w, e->qualified ? e->mask : 255u);
    field(&w, e->flags);
    field(&w, e->elapsed_ms);
    field(&w, e->drain_ms);
    field(&w, e->fill_ms);
    field(&w, e->cycles);
    field(&w, e->failed);
    field(&w, e->gaps);
    field(&w, e->drops);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(output, w.used);
    character(&w, '*');
    const char *hex = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    return w.failed ? 0 : w.used;
}
