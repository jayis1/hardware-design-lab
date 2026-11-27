/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void qualify(Engine *e, unsigned word)
{
    engine_sample(e, e->now + 1, (uint8_t)word);
    for (unsigned i = 0; i < 100; ++i)
        engine_sample(e, e->now + 1, (uint8_t)word);
    assert(engine_valid(e));
}

static void exhaustive(void)
{
    for (unsigned history = 0; history < 65536; ++history) {
        Engine e;
        engine_init(&e);
        e.settings = (uint16_t)history;
        unsigned supported = 0, rules = 0;
        unsigned expected[16];
        for (unsigned a = 0; a < 16; ++a) {
            unsigned matches = 0;
            unsigned absent[4] = {0, 0, 0, 0};
            for (unsigned w = 0; w < 16; ++w) {
                bool has_all = true;
                for (unsigned bit = 0; bit < 4; ++bit)
                    if ((a & (1u << bit)) && !(w & (1u << bit)))
                        has_all = false;
                if (!(history & (1u << w)) || !has_all)
                    continue;
                ++matches;
                for (unsigned bit = 0; bit < 4; ++bit)
                    if (!(w & (1u << bit)))
                        ++absent[bit];
            }
            unsigned closure = matches ? 0 : 16;
            for (unsigned bit = 0; matches && bit < 4; ++bit)
                if (!absent[bit])
                    closure |= 1u << bit;
            expected[a] = closure;
            assert(engine_support(&e, a) == matches);
            assert(engine_closure(&e, a) == closure);
            if (matches)
                supported |= 1u << a;
            for (unsigned bit = 0; bit < 4; ++bit) {
                unsigned w = engine_witness(&e, a, bit);
                assert((w != 16) == (absent[bit] != 0));
                if (w != 16) {
                    assert(history & (1u << w));
                    assert((w & a) == a);
                    assert(!(w & (1u << bit)));
                }
            }
        }
        for (unsigned bit = 0; bit < 4; ++bit) {
            unsigned minimal = 0;
            for (unsigned a = 0; a < 16; ++a) {
                if (a & (1u << bit)) continue;
                if (expected[a] == 16 || !(expected[a] & (1u << bit))) continue;
                bool redundant = false;
                for (unsigned b = 0; b < 16; ++b)
                    if (b != a && (a & b) == b && expected[b] != 16 &&
                        (expected[b] & (1u << bit))) redundant = true;
                if (!redundant) { minimal |= 1u << a; ++rules; }
            }
            assert(engine_minimal_for(&e, bit) == minimal);
        }
        assert(engine_supported(&e) == supported);
        assert(engine_rules(&e) == rules);
        assert(engine_project((uint16_t)history) == history);
    }
}

int main(void)
{
    exhaustive();
    for (unsigned a = 0; a < 16; ++a) {
        for (unsigned b = 0; b < 16; ++b) {
            Engine e; engine_init(&e);
            qualify(&e, a); qualify(&e, b);
            assert(e.settings == ((1u << a) | (1u << b)));
            assert(e.visits == (a == b ? 1u : 2u));
            assert(engine_closure(&e, 0) == (a & b));
        }
    }
    Engine e; engine_init(&e);
    engine_sample(&e, 10, 3);
    for (unsigned t = 11; t < 110; ++t) engine_sample(&e, t, 3);
    assert(!e.visits);
    engine_sample(&e, 110, 3); assert(e.visits == 1);
    engine_sample(&e, 111, 1); engine_sample(&e, 112, 3);
    assert(e.hold == 0 && e.visits == 1);
    engine_sample(&e, 117, 3);
    assert(e.gaps == 1 && e.missed == 4 && !e.hold);
    qualify(&e, 1);
    assert(engine_closure(&e, 1) == 1);
    assert(engine_witness(&e, 1, 1) == 1);
    assert(engine_closure(&e, 8) == 16);
    assert(engine_closure(&e, 16) == 16);
    assert(engine_witness(&e, 0, 4) == 16);
    assert(engine_minimal_for(&e, 4) == 0);
    assert(engine_support(&e, 16) == 0);
    char frame[1024];
    assert(wire_crc("123456789", 9) == 0x29b1);
    assert(wire_encode(&e, frame, sizeof frame));
    assert(strncmp(frame, "CB1,", 4) == 0);
    assert(!wire_encode(&e, frame, 4));
    assert(!wire_encode(&e, NULL, sizeof frame));
    Engine before = e;
    e.samples = UINT32_MAX;
    engine_sample(&e, e.now + 1, 2);
    assert(e.fault == F_EXHAUST && e.settings == before.settings);
    engine_init(&e); e.now = UINT32_MAX - 50; e.seeded = 1; e.samples = 1;
    qualify(&e, 0); assert(e.visits == 1 && engine_valid(&e));
    e.tx_errors = UINT32_MAX; engine_tx_error(&e); assert(e.tx_errors == UINT32_MAX);
    engine_fault(&e, F_CONFIG); engine_fault(&e, F_INPUT); assert(e.fault == F_CONFIG);
    before = e; engine_sample(&e, e.now + 1, 15);
    assert(e.samples == before.samples && e.settings == before.settings);
    engine_init(&e); engine_sample(&e, 1, 16); assert(e.fault == F_INPUT);
    engine_init(&e); e.settings = 65535;
    assert(engine_suggest(&e) == 16);
    puts("PASS: all 65536 histories, all antecedents/consequents, acquisition boundaries, CRC and faults");
}
