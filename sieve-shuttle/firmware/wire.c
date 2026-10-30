/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
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
typedef struct {
    char *data;
    size_t used, capacity;
    bool failed;
} Writer;
static void byte(Writer *w, char c)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
}
static void field(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned length = 0;
    byte(w, ',');
    do {
        reverse[length++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (length)
        byte(w, reverse[--length]);
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || !capacity || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    const char *marker = "SS1";
    while (*marker)
        byte(&w, *marker++);
    field(&w, e->now);
    field(&w, e->raw);
    field(&w, e->mask);
    field(&w, e->qualified);
    field(&w, e->state);
    field(&w, e->fault);
    field(&w, e->visits);
    field(&w, e->x);
    field(&w, e->y);
    field(&w, e->returns);
    field(&w, e->turns);
    field(&w, e->gaps);
    field(&w, e->tx_errors);
    field(&w, e->last);
    field(&w, e->pending);
    field(&w, e->clear_ms);
    field(&w, e->elapsed);
    field(&w, e->axis);
    field(&w, e->run);
    field(&w, e->peak_x);
    field(&w, e->peak_y);
    for (unsigned i = 0; i < 16; ++i)
        field(&w, e->matrix[i]);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    byte(&w, '*');
    for (unsigned shift = 16; shift; shift -= 4)
        byte(&w, "0123456789ABCDEF"[(crc >> (shift - 4)) & 15u]);
    byte(&w, '\n');
    size_t length = w.used;
    byte(&w, '\0');
    return w.failed ? 0 : length;
}
