/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>
typedef struct {
    char *buffer;
    size_t used;
    size_t capacity;
    uint16_t crc;
    bool failed;
} Writer;

static void octet(Writer *w, char c, bool checksum)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->buffer[w->used++] = c;
    if (!checksum)
        return;
    w->crc ^= (uint16_t)((uint16_t)(uint8_t)c << 8);
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (w->crc & 0x8000u)
            w->crc = (uint16_t)((w->crc << 1) ^ 0x1021u);
        else
            w->crc = (uint16_t)(w->crc << 1);
    }
}

static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    octet(w, ',', true);
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count)
        octet(w, reverse[--count], true);
}

size_t wire_encode(const Engine *e, char *destination, size_t capacity)
{
    /* Never emit an internally impossible snapshot. The main loop exposes
     * repeated encoder failures through the transport diagnostic counter. */
    if (!destination || !capacity || !engine_valid(e))
        return 0;
    Writer w = {destination, 0, capacity, 0xffffu, false};
    octet(&w, 'S', true);
    octet(&w, 'H', true);
    octet(&w, '1', true);
    number(&w, e->now);
    number(&w, e->phase);
    number(&w, e->fault);
    number(&w, e->raw);
    number(&w, e->mask);
    number(&w, e->valid);
    number(&w, e->age);
    number(&w, e->quiet);
    number(&w, e->count);
    number(&w, e->tx_errors);
    for (unsigned i = 0; i < MAX_EVENTS; ++i) {
        number(&w, e->event[i].at);
        number(&w, e->event[i].mask);
    }
    octet(&w, '*', false);
    const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        octet(&w, hex[(w.crc >> shift) & 15u], false);
    octet(&w, '\n', false);
    if (w.failed)
        return 0;
    return w.used;
}
