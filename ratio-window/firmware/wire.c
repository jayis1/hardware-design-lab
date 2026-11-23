/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

uint16_t wire_crc(const char *text, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)text[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (uint16_t)((crc << 1) ^ ((crc & 0x8000u) ? 0x1021u : 0u));
    }
    return crc;
}

typedef struct {
    char *data;
    size_t used;
    size_t capacity;
    bool failed;
} Writer;

static void put(Writer *w, char byte)
{
    if (w->used >= w->capacity) {
        w->failed = true;
        return;
    }
    w->data[w->used++] = byte;
}

static void number(Writer *w, uint32_t value)
{
    char reverse[10];
    unsigned length = 0;
    do {
        reverse[length++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (length)
        put(w, reverse[--length]);
}

size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    /* Validate before emitting: internal corruption must not look plausible. */
    if (!out || !capacity || !engine_valid(e))
        return 0;
    Writer w = {out, 0, capacity, false};
    const char marker[] = "RW1";
    for (unsigned i = 0; i < sizeof marker - 1u; ++i)
        put(&w, marker[i]);
    const uint32_t fields[] = {
        e->samples, e->now, e->state, e->fault, e->raw, e->qualified, e->valid,
        e->starts, e->good, e->bad, e->same, e->opposite, e->still,
        e->minimum, e->maximum, e->total, e->last, e->last_ms,
        e->last_input, e->last_output, e->steps, e->output, e->age,
        e->input_dir, e->output_dir, e->reason, e->quiet, e->tx_errors
    };
    for (unsigned i = 0; i < sizeof fields / sizeof fields[0]; ++i) {
        put(&w, ',');
        number(&w, fields[i]);
    }
    if (w.failed)
        return 0;
    uint16_t crc = wire_crc(out, w.used);
    static const char hex[] = "0123456789ABCDEF";
    put(&w, '*');
    for (int shift = 12; shift >= 0; shift -= 4)
        put(&w, hex[(crc >> shift) & 15]);
    put(&w, '\n');
    /* Reserve a diagnostic NUL that is not transmitted. */
    size_t length = w.used;
    put(&w, '\0');
    return w.failed ? 0 : length;
}
