/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <assert.h>
#include <stdio.h>

/* Independent bit-by-bit mismatch oracle, not XOR/popcount. */
static unsigned distance(unsigned a, unsigned b)
{
    unsigned result = 0;
    for (unsigned bit = 0; bit < 4; ++bit)
        result += ((a >> bit) & 1) != ((b >> bit) & 1);
    return result;
}
int main(void)
{
    for (unsigned history = 0; history < 65536; ++history) {
        Engine e;
        engine_init(&e);
        e.settings = (uint16_t)history;
        unsigned minimum = 5, ambiguous = 0;
        for (unsigned a = 0; a < 16; ++a) {
            if (!(history & (1u << a))) continue;
            for (unsigned b = a + 1; b < 16; ++b) {
                if (!(history & (1u << b))) continue;
                unsigned d = distance(a,b);
                if (d < minimum) minimum = d;
            }
        }
        for (unsigned reading = 0; reading < 16; ++reading) {
            unsigned candidates = 0, count = 0;
            for (unsigned code = 0; code < 16; ++code) {
                if ((history & (1u << code)) && distance(code,reading) <= 1) {
                    candidates |= 1u << code;
                    ++count;
                }
            }
            assert(engine_candidates(&e,reading) == candidates);
            if (count > 1) ambiguous |= 1u << reading;
        }
        assert(engine_distance(&e) == minimum);
        assert(engine_ambiguous(&e) == ambiguous);
        unsigned a = 99, b = 99;
        bool found = engine_witness(&e,&a,&b);
        assert(found == (minimum != 5));
        if (found) {
            assert(a < b && b < 16);
            assert((history & (1u << a)) && (history & (1u << b)));
            assert(distance(a,b) == minimum);
        }
    }
    Engine e;
    engine_init(&e);
    e.settings = 0x8001; /* Complementary 0000 / 1111. */
    assert(engine_distance(&e) == 4);
    assert(engine_ambiguous(&e) == 0);
    assert(engine_candidates(&e,3) == 0); /* Do not guess outside radius one. */
    assert(engine_candidates(&e,16) == 0);
    assert(!engine_witness(&e,0,0));
    e.settings |= 2;
    assert(engine_distance(&e) == 1); /* Extra code destroys old margin. */
    assert(engine_ambiguous(&e) & 3);
    puts("PASS: all 65536 codebooks, every reading and closest-pair witness");
}
