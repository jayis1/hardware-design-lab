/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *out;
    size_t capacity;
    size_t used;
    bool ok;
} Writer;

uint16_t wire_crc(const char *bytes, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)bytes[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0));
    }
    return crc;
}

static void put(Writer *w, char byte)
{
    if (!w->ok || w->used + 1 >= w->capacity) {
        w->ok = false;
        return;
    }
    w->out[w->used++] = byte;
}

static void number(Writer *w, int64_t value)
{
    char digits[24];
    unsigned used = 0;
    put(w, ',');
    if (value < 0) {
        put(w, '-');
        value = -value; /* All callers have bounded magnitude below INT64_MAX. */
    }
    do {
        digits[used++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (used)
        put(w, digits[--used]);
}

size_t wire_encode(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || !capacity)
        return 0;
    /* Avoid publishing impossible internal state after memory or logic faults. */
    if (e->state > INVALID || e->faults > 511u ||
        (e->faults && e->state != INVALID) ||
        e->elapsed > 120025u || e->span_um < 0 || e->span_um > 20000 ||
        e->peak_out_mn < 0 || e->peak_out_mn > 20000 ||
        e->peak_back_mn > 0 || e->peak_back_mn < -20000)
        return 0;
    Writer w = {out, capacity, 0, true};
    put(&w, 'D');
    put(&w, '1');
    number(&w, sequence);
    number(&w, e->now);
    number(&w, e->state);
    number(&w, e->faults);
    number(&w, e->acquired);
    for (unsigned i = 0; i < 4; ++i)
        number(&w, e->adc[i]);
    number(&w, e->base[0]);
    number(&w, e->base[1]);
    number(&w, e->x_um);
    number(&w, e->force_mn);
    number(&w, e->elapsed);
    number(&w, e->span_um);
    number(&w, e->peak_out_mn);
    number(&w, e->peak_back_mn);
    number(&w, e->out_nj2 / 2000);
    number(&w, e->back_nj2 / 2000);
    if (!w.ok)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    put(&w, ',');
    static const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15u]);
    put(&w, '\n');
    if (!w.ok)
        return 0;
    out[w.used] = 0;
    return w.used;
}
