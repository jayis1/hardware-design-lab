/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
#include <stdbool.h>
/* CRC-16/CCITT-FALSE: polynomial 1021, initial FFFF, no reflection/xorout. */
uint16_t wire_crc(const char *data, size_t length)
{
    uint16_t crc = 0xffffu;
    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)(uint8_t)data[i] << 8;
        for (unsigned bit = 0; bit < 8; ++bit) {
            bool top = (crc & 0x8000u) != 0;
            crc = (uint16_t)(crc << 1);
            if (top)
                crc ^= 0x1021u;
        }
    }
    return crc;
}
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
    bool failed;
} Writer;
static void character(Writer *writer, char value)
{
    if (writer->length >= writer->capacity) {
        writer->failed = true;
        return;
    }
    writer->data[writer->length++] = value;
}
static void decimal(Writer *writer, uint32_t value)
{
    char reverse[10];
    unsigned count = 0;
    do {
        reverse[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    character(writer, ',');
    while (count)
        character(writer, reverse[--count]);
}
size_t wire_encode(const Engine *e, char *out, size_t capacity)
{
    if (!out || !capacity || !engine_valid(e))
        return 0;
    Writer writer = {out, 0, capacity, false};
    character(&writer, 'R');
    character(&writer, 'R');
    character(&writer, '1');
    decimal(&writer, e->now);
    decimal(&writer, e->state);
    decimal(&writer, e->raw);
    decimal(&writer, e->stable);
    decimal(&writer, e->qualified);
    decimal(&writer, e->faults);
    decimal(&writer, e->age);
    decimal(&writer, e->high);
    decimal(&writer, e->downmask);
    decimal(&writer, e->upmask);
    decimal(&writer, e->events);
    decimal(&writer, e->gaps);
    decimal(&writer, e->tx_errors);
    decimal(&writer, e->clear_ms);
    for (unsigned i = 0; i < 4; ++i)
        decimal(&writer, e->down[i]);
    for (unsigned i = 0; i < 4; ++i)
        decimal(&writer, e->up[i]);
    decimal(&writer, e->end_ms);
    if (writer.failed)
        return 0;
    uint16_t crc = wire_crc(out, writer.length);
    character(&writer, '*');
    const char *hex = "0123456789ABCDEF";
    for (int shift = 12; shift >= 0; shift -= 4)
        character(&writer, hex[(crc >> shift) & 15u]);
    character(&writer, '\n');
    /* Returned data is a byte span, deliberately not a NUL-terminated string. */
    return writer.failed ? 0 : writer.length;
}
