/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include <assert.h>
#include <stdio.h>
/* Independent oracle scans actual observations for each candidate; production
 * uses packed projection membership. Python tests also use a pair-splice oracle. */
int main(void)
{
    for (unsigned h = 0; h < 65536; ++h) {
        Engine e;
        engine_init(&e);
        e.settings = (uint16_t)h;
        unsigned complete = 0, total = 0;
        for (unsigned s = 1; s < 15; s += 2) {
            unsigned expected = 0;
            for (unsigned candidate = 0; candidate < 16; ++candidate) {
                bool left = false, right = false;
                for (unsigned w = 0; w < 16; ++w) {
                    if (!(h & (1u << w))) continue;
                    if ((w & s) == (candidate & s)) left = true;
                    if ((w & (15 ^ s)) == (candidate & (15 ^ s))) right = true;
                }
                if (left && right && !(h & (1u << candidate)))
                    expected |= 1u << candidate;
            }
            assert(engine_missing(&e, s) == expected);
            if (h && !expected) complete |= 1u << (s / 2);
            total += engine_popcount(expected);
            for (unsigned w = 0; w < 16; ++w) {
                unsigned pair = engine_parents(&e, s, w);
                if (!(expected & (1u << w))) {
                    assert(pair == 256);
                } else {
                    assert(pair < 256);
                    unsigned a = pair / 16, b = pair % 16;
                    assert(h & (1u << a)); assert(h & (1u << b));
                    assert(((a & s) | (b & (15 ^ s))) == w);
                }
            }
        }
        assert(engine_complete(&e) == complete);
        assert(engine_missing_total(&e) == total);
        unsigned suggestion = engine_suggest(&e);
        if (h == 65535) assert(suggestion == 16);
        else assert(suggestion < 16 && !(h & (1u << suggestion)));
    }
    Engine e; engine_init(&e);
    assert(engine_complete(&e) == 0);
    assert(engine_missing(&e, 0) == 0);
    assert(engine_missing(&e, 15) == 0);
    assert(engine_parents(&e, 3, 16) == 256);
    e.settings = 1u | (1u << 15);
    assert(engine_missing(&e, 3) == ((1u << 3) | (1u << 12)));
    e.settings |= (1u << 3) | (1u << 12);
    assert(engine_missing(&e, 3) == 0);
    assert(engine_complete(&e) & 2u);
    puts("PASS: 65536 histories, seven bipartitions, explicit recombination parents");
}
