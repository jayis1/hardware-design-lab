/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <limits.h>

typedef struct {
    char *data;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void character(Writer *w, char c)
{
    if (w->failed)
        return;
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = c;
}

static void text(Writer *w, const char *s)
{
    while (*s)
        character(w, *s++);
}

static void decimal(Writer *w, uint64_t number)
{
    char digits[20];
    size_t size = 0;
    do {
        digits[size++] = (char)('0' + number % 10u);
        number /= 10u;
    } while (number);
    while (size)
        character(w, digits[--size]);
}

static void signed_decimal(Writer *w, int64_t number)
{
    uint64_t magnitude;
    if (number < 0) {
        character(w, '-');
        /* Avoid signed overflow even for INT64_MIN. */
        magnitude = (uint64_t)(-(number + 1)) + 1u;
    } else {
        magnitude = (uint64_t)number;
    }
    decimal(w, magnitude);
}

static void field(Writer *w, uint32_t number)
{
    character(w, ',');
    decimal(w, number);
}

static void signed_field(Writer *w, int64_t number)
{
    character(w, ',');
    signed_decimal(w, number);
}

uint16_t wire_crc(const char *data, size_t size)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < size; ++i) {
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

static void checksum(Writer *w)
{
    static const char hex[] = "0123456789ABCDEF";
    if (w->failed)
        return;
    uint16_t crc = wire_crc(w->data, w->used);
    character(w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        character(w, hex[(crc >> shift) & 15u]);
    character(w, '\n');
}

size_t wire_encode(const Engine *e, uint32_t sequence, uint32_t transport_errors,
                   char *buffer, size_t capacity)
{
    if (!e || !buffer || capacity < 8)
        return 0;
    Writer w = { buffer, capacity, 0, false };
    text(&w, "WSL1");
    field(&w, sequence);
    field(&w, e->time);
    signed_field(&w, e->axis[0].position);
    signed_field(&w, e->axis[1].position);
    signed_field(&w, engine_difference(e));
    field(&w, e->axis[0].invalid);
    field(&w, e->axis[1].invalid);
    field(&w, e->gaps);
    field(&w, e->reversals);
    signed_field(&w, e->leg_peak);
    field(&w, e->previous_leg_ms);
    field(&w, e->idle_ms);
    field(&w, engine_flags(e));
    field(&w, transport_errors);
    checksum(&w);
    if (w.failed)
        return 0;
    return w.used;
}
