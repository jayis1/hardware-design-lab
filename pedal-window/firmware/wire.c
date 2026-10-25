/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"

typedef struct {
    char *out;
    size_t capacity;
    size_t used;
    bool failed;
} Writer;

static void character(Writer *writer, char value)
{
    if (writer->used >= writer->capacity) {
        writer->failed = true;
        return;
    }
    writer->out[writer->used++] = value;
}

static void text(Writer *writer, const char *value)
{
    while (*value) {
        character(writer, *value++);
    }
}

static void number(Writer *writer, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    while (count) {
        character(writer, reverse[--count]);
    }
}

static void field(Writer *writer, uint32_t value)
{
    character(writer, ',');
    number(writer, value);
}

static void signed_field(Writer *writer, int32_t value)
{
    character(writer, ',');
    if (value < 0) {
        character(writer, '-');
        /* Work in the unsigned domain, including INT32_MIN. */
        number(writer, 0u - (uint32_t)value);
    } else {
        number(writer, (uint32_t)value);
    }
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

static void hexadecimal(Writer *writer, uint16_t value)
{
    const char *digits = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4) {
        character(writer, digits[(value >> shift) & 15u]);
    }
}

size_t wire_status(const Engine *e, uint32_t sequence, char *out, size_t capacity)
{
    if (!e || !out || !capacity) {
        return 0;
    }
    Writer writer = {out, capacity, 0, false};
    text(&writer, "PW1");
    field(&writer, sequence);
    field(&writer, e->now);
    field(&writer, engine_flags(e));
    field(&writer, e->levels);
    field(&writer, (uint32_t)e->phase);
    field(&writer, e->attempts);
    field(&writer, e->completed);
    field(&writer, e->rejected);
    field(&writer, e->gaps);
    field(&writer, e->transport_losses);
    field(&writer, e->last.id);
    field(&writer, e->last.outcome);
    field(&writer, e->last.flags);
    signed_field(&writer, e->last.key_gap);
    signed_field(&writer, e->last.release_delta);
    signed_field(&writer, e->last.down_delta);
    /* Policy values repeat so attaching after boot remains unambiguous. */
    field(&writer, DEBOUNCE_MS);
    field(&writer, TRIAL_MS);
    if (writer.failed) {
        return 0;
    }
    uint16_t crc = wire_crc(out, writer.used);
    character(&writer, '*');
    hexadecimal(&writer, crc);
    character(&writer, '\n');
    if (writer.failed) {
        return 0;
    }
    return writer.used;
}
