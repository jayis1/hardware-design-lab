/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *text;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;
static void put(Writer *w, char c)
{
    if (w->used + 1u >= w->capacity) {
        w->failed = true;
        return;
    }
    w->text[w->used++] = c;
}
static void number(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned n = 0;
    do {
        digits[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    put(w, ',');
    while (n)
        put(w, digits[--n]);
}
uint16_t wire_crc(const char *text, size_t length)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)text[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}
static void slot(Writer *w, const Slot *s)
{
    number(w, s->state);
    number(w, s->starts);
    number(w, s->early);
    number(w, s->rested);
    number(w, s->aborted);
    number(w, s->age);
    number(w, s->last);
    number(w, s->minimum);
    number(w, s->maximum);
    number(w, s->total);
    number(w, s->timeouts);
}
size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!buffer || capacity < 8u || !engine_valid(e))
        return 0;
    Writer w = {buffer, capacity, 0, false};
    put(&w, 'R');
    put(&w, 'Q');
    put(&w, '1');
    number(&w, e->now);
    number(&w, e->gaps);
    number(&w, e->tx);
    number(&w, e->fault);
    number(&w, QUALIFY_MS);
    number(&w, GOAL_MS);
    number(&w, LIMIT_MS);
    for (unsigned i = 0; i < CHANNELS; ++i)
        slot(&w, &e->slot[i]);
    uint16_t crc = wire_crc(buffer, w.used);
    put(&w, '*');
    const char *hex = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15u]);
    put(&w, '\n');
    if (w.failed)
        return 0;
    buffer[w.used] = '\0';
    return w.used;
}
