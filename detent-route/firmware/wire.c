/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *data;
    size_t used;
    size_t capacity;
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

static void number(Writer *w, uint32_t n)
{
    char digits[10];
    size_t used = 0;
    do {
        digits[used++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    character(w, ',');
    while (used)
        character(w, digits[--used]);
}

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffff;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000)
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

size_t wire_encode(Engine *e, char *out, size_t capacity)
{
    if (!out || !capacity || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    character(&w, 'D');
    character(&w, 'R');
    character(&w, '1');
    number(&w, e->sequence);
    number(&w, e->now);
    number(&w, e->fault);
    number(&w, e->raw);
    number(&w, e->qualified);
    number(&w, e->ready);
    number(&w, e->station);
    number(&w, e->active);
    number(&w, e->origin);
    number(&w, e->age);
    number(&w, e->starts);
    number(&w, e->cw);
    number(&w, e->ccw);
    number(&w, e->returns);
    number(&w, e->jumps);
    number(&w, e->aborted);
    number(&w, e->reversals);
    number(&w, e->cw_laps);
    number(&w, e->ccw_laps);
    number(&w, e->overlaps);
    number(&w, e->shortcuts);
    number(&w, e->timeouts);
    number(&w, e->gaps);
    number(&w, e->tx_errors);
    number(&w, e->direction);
    number(&w, e->run);
    number(&w, e->last_valid);
    number(&w, e->last_from);
    number(&w, e->last_to);
    number(&w, e->last_ms);
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    character(&w, '*');
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&w, hex[(crc >> shift) & 15]);
    character(&w, '\n');
    if (w.failed)
        return 0;
    ++e->sequence;
    return w.used;
}
