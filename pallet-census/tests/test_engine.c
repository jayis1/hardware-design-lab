/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
    }
}
static Engine begin(unsigned pallet)
{
    Engine e;
    engine_init(&e);
    hold(&e, pallet, 60);
    assert(e.state == READY);
    hold(&e, pallet | 8, 5);
    assert(e.state == ACTIVE && e.starts == 1 && e.age == 0);
    return e;
}
static void pulse(Engine *e, unsigned pallet)
{
    hold(e, pallet | 8, 5);
    hold(e, pallet | 12, 5);
    hold(e, pallet | 8, 5);
}
static void oracle_words(void)
{
    /* Independent last-letter walk over all 4^6 stable pallet words.
     * Invalid lock codes break adjacency; they do not disappear. */
    for (unsigned word = 0; word < 4096; ++word) {
        Engine e = begin(0);
        unsigned counts[4] = {0}, pairs[4] = {0}, prev = 0, w = word;
        for (unsigned j = 0; j < 6; ++j) {
            unsigned p = w % 4; w /= 4;
            ++counts[p];
            if (p == 1 || p == 2) {
                if (prev) ++pairs[(prev - 1) * 2 + p - 1];
                prev = p;
            } else prev = 0;
            pulse(&e, p);
            assert(e.pulses == j + 1);
            assert(e.a == counts[1] && e.b == counts[2]);
            assert(e.neither == counts[0] && e.both == counts[3]);
            assert(e.aa == pairs[0] && e.ab == pairs[1]);
            assert(e.ba == pairs[2] && e.bb == pairs[3]);
            assert(e.anchor == prev && !e.uncertain);
        }
        hold(&e, 0, 5);
        assert(e.completed == 1 && e.state == WAIT);
    }
}
int main(void)
{
    Engine e;
    engine_init(&e);
    hold(&e, 0, 54); assert(e.state == WAIT && e.baseline == 49);
    hold(&e, 0, 1); assert(e.state == READY);
    engine_init(&e);
    hold(&e, 8, 100); assert(e.state == WAIT && !e.starts);
    hold(&e, 0, 55); assert(e.state == READY);
    e = begin(1);
    /* A -> B simultaneous with INDEX qualification is uncertain. */
    hold(&e, 14, 5);
    assert(e.uncertain == 1 && !e.a && !e.b && !e.anchor);
    e = begin(1);
    hold(&e, 13, 4);
    hold(&e, 12, 1); /* raw A loss on qualified index sample */
    assert(e.uncertain == 1);
    e = begin(1);
    hold(&e, 13, 4);
    hold(&e, 5, 1); /* raw RUN loss on index endpoint */
    assert(e.state == HALT && e.fault == F_BOUNDARY && e.aborted == 1);
    e = begin(1);
    engine_sample(&e, e.now + 2, 9);
    assert(e.state == HALT && e.gaps == 1 && e.aborted == 1);
    assert(engine_valid(&e));
    e = begin(1);
    e.age = DEADLINE - 5;
    hold(&e, 1, 5);
    assert(e.state == HALT && e.fault == F_DEADLINE && e.age == DEADLINE);
    e = begin(1);
    e.age = DEADLINE - 6;
    hold(&e, 1, 5);
    assert(e.state == WAIT && e.completed == 1 && e.age == DEADLINE - 1);
    e = begin(0);
    /* Independent qualification survives unrelated pallet chatter. */
    for (unsigned i = 0; i < 5; ++i) hold(&e, 12 | (i & 1), 1);
    assert(e.pulses == 1);
    e = begin(1);
    e.now = 0xfffffffeu;
    hold(&e, 9, 3); assert(e.now == 1 && e.state == ACTIVE);
    e = begin(1);
    e.sequence = UINT32_MAX;
    hold(&e, 9, 1); assert(e.fault == F_COUNTER && e.aborted == 1);
    e = begin(1);
    char output[768];
    assert(wire_encode(&e, output, sizeof output));
    assert(!wire_encode(&e, output, 4));
    assert(wire_crc("123456789", 9) == 0x29b1);
    e.pulses = 1;
    assert(!wire_encode(&e, output, sizeof output));
    e = begin(1);
    e.age = 30; e.pulses = 3; e.a = 2; e.neither = 1;
    e.aa = 1; e.ba = 1; e.anchor = 1;
    assert(!wire_encode(&e, output, sizeof output));
    oracle_words();
    puts("PASS engine boundaries, independent qualification, 4096 oracle words, wire checks");
}
