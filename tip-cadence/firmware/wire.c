/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *out;
    size_t used, capacity;
    bool failed;
} Writer;
static void character(Writer *w, char c)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = c;
}
static void decimal(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned n = 0;
    do {
        reverse[n++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(w, ',');
    while (n)
        character(w, reverse[--n]);
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
static void lane(Writer *w, const Lane *l)
{
    decimal(w, l->tips);
    decimal(w, l->intervals);
    decimal(w, l->bins[0]);
    decimal(w, l->bins[1]);
    decimal(w, l->sum[0]);
    decimal(w, l->sum[1]);
    decimal(w, l->minimum);
    decimal(w, l->maximum);
    decimal(w, l->jitter);
    decimal(w, l->rejected);
    decimal(w, l->breaks);
    decimal(w, l->phase);
    decimal(w, l->anchored);
    decimal(w, l->since_tip);
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!e || !out || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    const char *identity = "TC1";
    while (*identity)
        character(&w, *identity++);
    decimal(&w, e->now);
    decimal(&w, e->raw);
    decimal(&w, e->gaps);
    decimal(&w, e->tx_errors);
    decimal(&w, e->faults);
    lane(&w, &e->lane[0]);
    lane(&w, &e->lane[1]);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    character(&w, '*');
    const char *hex = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    return w.failed ? 0 : w.used;
}
