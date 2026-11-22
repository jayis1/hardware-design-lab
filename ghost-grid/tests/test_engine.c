/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"

/* Independent graph reachability oracle, not the engine's population rule. */
static unsigned scan(unsigned mask)
{
    unsigned result = 0;
    for (unsigned row = 0; row < 2; ++row) {
        unsigned reached = 1u << row;
        for (unsigned pass = 0; pass < 4; ++pass)
            for (unsigned key = 0; key < 4; ++key) {
                unsigned edge = (1u << (key / 2)) | (1u << (2 + key % 2));
                if ((mask & (1u << key)) && (reached & edge))
                    reached |= edge;
            }
        for (unsigned col = 0; col < 2; ++col)
            if (reached & (1u << (2 + col)))
                result |= 1u << (row * 2 + col);
    }
    return result;
}
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
    }
}
int main(void)
{
    Engine e;
    for (unsigned a = 0; a < 16; ++a) {
        assert(matrix_scan(a) == scan(a));
        assert(matrix_ghost(a) == (scan(a) & ~a));
        for (unsigned b = 0; b < 16; ++b) {
            engine_init(&e);
            hold(&e, a, 6);
            assert(e.unknown == 6 && e.bins[a] == 0);
            hold(&e, a, 4);
            assert(e.bins[a] == 4 && e.segments[a] == 1 && e.peaks[a] == 4);
            hold(&e, b, 10);
            assert(e.bins[b] == (a == b ? 14u : 4u));
            assert(e.fault == 0);
        }
    }
    engine_init(&e);
    hold(&e, 7, 10);
    hold(&e, 3, 1); /* Rejected glitch splits continuity nevertheless. */
    hold(&e, 7, 10);
    assert(e.bins[7] == 8 && e.segments[7] == 2 && e.peaks[7] == 4);
    engine_sample(&e, e.now + 11, 7);
    assert(e.missed == 10 && e.run == 0);
    hold(&e, 7, 6);
    assert(e.bins[7] == 9 && e.segments[7] == 3);
    e.now = UINT32_MAX - 2;
    hold(&e, 7, 5);
    assert(e.fault == 0 && e.now == 2);
    Engine previous = e;
    engine_sample(&e, e.now, 3);
    assert(e.fault == F_TIME && e.samples == previous.samples);
    hold(&e, 15, 10);
    assert(e.bins[7] == previous.bins[7]);
    engine_init(&e);
    e.samples = UINT32_MAX - 1;
    e.unknown = UINT32_MAX - 1;
    hold(&e, 0, 1);
    assert(e.samples == UINT32_MAX && e.fault == 0);
    hold(&e, 0, 1);
    assert(e.samples == UINT32_MAX && e.fault == F_OVERFLOW);
    engine_init(&e);
    engine_sample(&e, 1, 16);
    assert(e.fault == F_INPUT && e.samples == 0);
    engine_init(&e);
    hold(&e, 14, 20);
    char line[768];
    size_t n = wire_encode(&e, line, sizeof line);
    assert(n > 0 && line[n-1] == '\n');
    assert(wire_encode(&e, line, 10) == 0);
    assert(wire_crc("123456789", 9) == 0x29b1);
    e.peaks[14] = 0;
    assert(!engine_valid(&e) && !wire_encode(&e, line, sizeof line));
    for (unsigned count = 0; count < 6; ++count) {
        engine_init(&e);
        e.samples = count;
        e.unknown = count;
        e.raw = e.qualified = 7;
        e.valid = 1;
        assert(!engine_valid(&e));
        assert(!wire_encode(&e, line, sizeof line));
    }
    puts("PASS all 256 mask transitions, graph oracle, glitch/gap/wrap/fault/overflow/framing");
}
