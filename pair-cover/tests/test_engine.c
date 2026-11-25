/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned count)
{
    while (count--) engine_sample(e, ++now, (uint8_t)mask);
    assert(engine_valid(e));
}
static uint32_t oracle(unsigned mask)
{
    uint32_t coverage = 0;
    unsigned pair = 0;
    for (unsigned a = 0; a < 4; ++a)
        for (unsigned b = a + 1; b < 4; ++b, ++pair)
            coverage |= 1u << (pair * 4 + ((mask >> a) & 1) * 2 + ((mask >> b) & 1));
    return coverage;
}
int main(void)
{
    /* Every subset of the sixteen physical settings has exact pair projection. */
    for (unsigned subset = 0; subset < 65536; ++subset) {
        uint32_t expected = 0;
        for (unsigned mask = 0; mask < 16; ++mask)
            if (subset & (1u << mask)) expected |= oracle(mask);
        assert(engine_project((uint16_t)subset) == expected);
    }
    for (unsigned a = 0; a < 16; ++a) {
        for (unsigned b = 0; b < 16; ++b) {
            Engine e; engine_init(&e); now = 0;
            hold(&e, a, HOLD_MS);
            assert(e.settings == 0); /* First observation has no preceding interval. */
            hold(&e, a, 1);
            assert(e.settings == (1u << a));
            hold(&e, b, HOLD_MS + 1);
            assert(e.settings == ((1u << a) | (1u << b)));
            assert(e.coverage == (oracle(a) | oracle(b)));
            assert(e.visits == (a == b ? 1u : 2u));
        }
    }
    Engine e; engine_init(&e); now = 0;
    hold(&e, 0, HOLD_MS);
    hold(&e, 1, 1); hold(&e, 0, HOLD_MS);
    assert(e.coverage == 0); /* One-sample raw interruption invalidates dwell. */
    hold(&e, 0, 1); assert(e.visits == 1);
    uint32_t old = e.coverage;
    now += 50; engine_sample(&e, ++now, 15);
    assert(e.gaps == 1 && e.missed == 50 && e.hold == 0);
    assert(e.coverage == old);
    hold(&e, 15, HOLD_MS - 1); assert(e.visits == 1);
    hold(&e, 15, 1); assert(e.visits == 2);
    for (unsigned mask = 0; mask < 16; ++mask) hold(&e, mask, HOLD_MS + 1);
    assert(e.coverage == COVER_ALL && e.settings == 65535);
    assert(engine_suggest(&e) == 16);
    char frame[1024];
    assert(wire_encode(&e, frame, sizeof frame) > 0);
    assert(!strncmp(frame, "PC1,", 4));
    assert(wire_crc("123456789", 9) == 0x29b1);
    assert(wire_encode(&e, frame, 3) == 0);
    engine_init(&e); now = UINT32_MAX - 50;
    hold(&e, 3, HOLD_MS + 1); assert(e.visits == 1);
    e.samples = UINT32_MAX;
    e.unknown_ms = UINT32_MAX - 1;
    hold(&e, 0, 1); assert(e.fault == F_EXHAUST);
    old = e.coverage;
    hold(&e, 15, 500); assert(e.coverage == old);
    engine_init(&e); engine_sample(&e, 1, 16);
    assert(e.fault == F_INPUT && engine_valid(&e));
    engine_init(&e); now=0; hold(&e, 0, 1);
    engine_sample(&e, now, 15); assert(e.raw == 0); /* Duplicate tick ignored. */
    for (unsigned subset = 0; subset < 65536; subset += 7) {
        engine_init(&e); e.settings = (uint16_t)subset;
        e.coverage = engine_project(e.settings);
        e.raw = (uint8_t)(subset & 15);
        unsigned pick = engine_suggest(&e);
        if (e.coverage == COVER_ALL) assert(pick == 16);
        else {
            assert(pick < 16);
            assert(oracle(pick) & ~e.coverage);
            unsigned cost = engine_popcount(pick ^ e.raw);
            for (unsigned m=0; m<16; ++m)
                if (oracle(m) & ~e.coverage)
                    assert(cost <= engine_popcount(m ^ e.raw));
        }
    }
    puts("PASS pair projection 65536 subsets; all 256 transitions; timing, gap, wrap, overflow, wire, suggestions");
}
