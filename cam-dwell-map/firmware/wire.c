/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "wire.h"
/* No printf, heap, hidden truncation or locale in the target protocol. */
typedef struct {
    char *out;
    size_t size;
    size_t used;
    uint16_t crc;
    bool failed;
} Writer;

static void byte(Writer *w, char c, bool checksum)
{
    if (w->used >= w->size) {
        w->failed = true;
        return;
    }
    w->out[w->used++] = c;
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
    char digits[10];
    unsigned used = 0;
    do {
        digits[used++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value);
    byte(w, ',', true);
    while (used)
        byte(w, digits[--used], true);
}

size_t cam_encode(const Cam *e, uint32_t now, uint32_t tx_errors,
                  char *output, size_t capacity)
{
    if (!output || !capacity || !cam_valid(e))
        return 0;
    Writer w = {output, capacity, 0, 0xffffu, false};
    const char *prefix = "CDM1";
    while (*prefix)
        byte(&w, *prefix++, true);
    number(&w, now);
    number(&w, e->state);
    number(&w, e->raw);
    number(&w, e->follower);
    number(&w, e->travel);
    number(&w, e->completed);
    number(&w, e->rejected);
    number(&w, e->gaps);
    number(&w, e->reason);
    number(&w, e->result.duration);
    number(&w, e->result.closed);
    number(&w, e->result.transitions);
    number(&w, tx_errors);
    /* One hexadecimal digit per bin; legal counts are zero through ten.
     * Bin vector is always the most recent COMPLETE turn, never partial. */
    byte(&w, ',', true);
    const char *hex = "0123456789ABCDEF";
    for (unsigned i = 0; i < CAM_BINS; ++i)
        byte(&w, hex[e->result.bins[i]], true);
    uint16_t crc = w.crc;
    byte(&w, '*', false);
    for (int shift = 12; shift >= 0; shift -= 4)
        byte(&w, hex[(crc >> shift) & 15u], false);
    byte(&w, '\n', false);
    return w.failed ? 0 : w.used;
}
