/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
typedef struct {
    char *text;
    size_t capacity, used;
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
size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!buffer || capacity < 8u || !engine_valid(e))
        return 0;
    Writer w = {buffer, capacity, 0, false};
    put(&w, 'F'); put(&w, 'P'); put(&w, '1');
    number(&w, e->now);
    number(&w, e->gaps);
    number(&w, e->tx);
    number(&w, e->fault);
    number(&w, QUALIFY_MS);
    number(&w, SILENCE_MS);
    number(&w, MAX_PULSE_MS);
    number(&w, e->state);
    number(&w, e->mask);
    number(&w, e->known);
    number(&w, e->runs);
    number(&w, e->finished);
    number(&w, e->aborted);
    number(&w, e->pulses);
    number(&w, e->bad_pulses);
    number(&w, e->pauses);
    number(&w, e->recovered);
    number(&w, e->assisted);
    number(&w, e->abandoned);
    number(&w, e->taps);
    number(&w, e->silent);
    number(&w, e->pause_age);
    number(&w, e->pause_total);
    number(&w, e->pause_max);
    number(&w, e->episode_taps);
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
