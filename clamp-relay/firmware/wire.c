/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>

/* CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xffff, no reflection.
 * This detects accidental corruption; it does not authenticate the source. */
uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= (uint16_t)((uint8_t)data[i] << 8);
        for (unsigned bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000u)
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            else
                crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

typedef struct {
    char *data;
    size_t used;
    size_t capacity;
    bool valid;
} Writer;

static void byte(Writer *w, char value)
{
    /* Reserve a NUL for diagnostic host use, not sent over the wire. */
    if (w->used + 1 >= w->capacity) {
        w->valid = false;
        return;
    }
    w->data[w->used++] = value;
}

static void text(Writer *w, const char *value)
{
    while (*value)
        byte(w, *value++);
}

static void decimal(Writer *w, uint32_t value)
{
    char digits[10];
    unsigned used = 0;
    do {
        digits[used++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (used)
        byte(w, digits[--used]);
}

static void field(Writer *w, uint32_t value)
{
    byte(w, ',');
    decimal(w, value);
}

size_t wire_encode(const Engine *e, char *buffer, size_t capacity)
{
    if (!e || !buffer || capacity < 2 || !engine_valid(e))
        return 0;
    Writer w = {buffer, 0, capacity, true};
    /* Policy constants repeat in every record, permitting late attachment. */
    text(&w, "CR1");
    field(&w, QUALIFY_MS);
    field(&w, SETTLE_MS);
    field(&w, TRIAL_MS);
    field(&w, e->now);
    field(&w, e->levels);
    field(&w, e->qualified);
    field(&w, e->grips);
    field(&w, e->owner);
    field(&w, e->result);
    field(&w, e->direction);
    field(&w, e->starts);
    field(&w, e->completed);
    field(&w, e->aborted);
    field(&w, e->elapsed_ms);
    field(&w, e->gap_ms);
    field(&w, e->overlap_ms);
    field(&w, e->max_gap_ms);
    field(&w, e->direct);
    field(&w, e->gaps);
    field(&w, e->tx_errors);
    field(&w, e->faults);
    if (!w.valid)
        return 0;
    uint16_t crc = wire_crc(buffer, w.used);
    byte(&w, '*');
    static const char hex[] = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15u]);
    byte(&w, '\n');
    if (!w.valid)
        return 0;
    buffer[w.used] = '\0';
    return w.used;
}
