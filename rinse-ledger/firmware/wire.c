/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *out;
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
    w->out[w->used++] = value;
}
static void text(Writer *w, const char *value)
{
    while (*value) {
        character(w, *value++);
    }
}
static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count) {
        character(w, reverse[--count]);
    }
}
static void field(Writer *w, uint32_t value)
{
    character(w, ',');
    number(w, value);
}
uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)((uint16_t)(uint8_t)data[i] << 8);
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
static void hexadecimal(Writer *w, uint16_t value)
{
    const char *digits = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4) {
        character(w, digits[(value >> shift) & 15u]);
    }
}
static void basket_fields(Writer *w, const Basket *b)
{
    field(w, b->total);
    field(w, b->epoch_ms);
    field(w, b->streak);
    field(w, b->longest);
    field(w, b->entries);
    field(w, b->baths);
    field(w, b->flags);
}
size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || !capacity) {
        return 0;
    }
    Writer w = {out, capacity, 0, false};
    text(&w, "RL1");
    field(&w, sequence);
    field(&w, e->now);
    field(&w, e->ready ? 1u : 0u);
    field(&w, e->levels);
    field(&w, e->epoch);
    field(&w, e->rejected);
    field(&w, e->gaps);
    field(&w, e->losses);
    field(&w, DEBOUNCE_MS);
    field(&w, MAX_GAP_MS);
    for (unsigned i = 0; i < BASKETS; ++i) {
        basket_fields(&w, &e->basket[i]);
    }
    if (w.failed) {
        return 0;
    }
    uint16_t crc = wire_crc(out, w.used);
    character(&w, '*');
    hexadecimal(&w, crc);
    character(&w, '\n');
    if (w.failed) {
        return 0;
    }
    return w.used;
}
