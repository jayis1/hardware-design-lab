/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "engine.h"
#include "wire.h"

static void hold(Engine *e, unsigned word, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, word);
        assert(engine_valid(e));
    }
}
/* An independent parity oracle counts occupied holes, not XOR folds. */
static unsigned odd(unsigned word)
{
    unsigned count = 0;
    for (unsigned bit = 0; bit < 4; ++bit)
        if (word & (1u << bit)) ++count;
    return count % 2;
}
int main(void)
{
    Engine e;
    for (unsigned a = 0; a < 16; ++a) {
        assert(parity_odd(a) == odd(a));
        for (unsigned b = 0; b < 16; ++b) {
            for (unsigned c = 0; c < 16; ++c) {
                engine_init(&e);
                unsigned active = 0, known = 0, anchor = 0;
                unsigned starts = 0, same = 0, changed = 0, orphan = 0;
                unsigned sequence[] = {a,b,c};
                for (unsigned j = 0; j < 3; ++j) {
                    unsigned word = sequence[j];
                    if (odd(word)) {
                        if (!active) { active = 1; ++starts; }
                    } else {
                        if (active) {
                            if (!known) ++orphan;
                            else if (anchor == word) ++same;
                            else ++changed;
                            active = 0;
                        }
                        known = 1; anchor = word;
                    }
                    hold(&e, word, 10);
                    assert(e.active == active && e.starts == starts);
                    assert(e.same == same && e.changed == changed && e.orphan == orphan);
                    assert(e.known == known && e.anchor == anchor);
                }
            }
        }
    }
    engine_init(&e);
    hold(&e, 0, 10);
    hold(&e, 1, 5); /* No qualified odd word, hence no episode. */
    assert(e.starts == 0);
    hold(&e, 0, 10);
    assert(e.starts == 0 && e.segments[0] == 2);
    hold(&e, 1, 6);
    assert(e.active == 1 && e.age == 0);
    hold(&e, 2, 10); /* Odd-to-odd transition does not end the episode. */
    assert(e.starts == 1 && e.age == 10);
    hold(&e, 0, 6);
    assert(e.same == 1 && e.last_age == 16 && e.active == 0);
    hold(&e, 1, 10);
    engine_sample(&e, e.now + 11, 1);
    assert(e.aborted == 1 && !e.active && !e.known && e.missed == 10);
    assert(engine_valid(&e));
    hold(&e, 1, 5);
    assert(e.active && !e.known);
    hold(&e, 3, 6);
    assert(e.orphan == 1);
    e.now = UINT32_MAX - 2;
    hold(&e, 3, 5);
    assert(!e.fault && e.now == 2);
    hold(&e, 1, 10);
    unsigned old_samples = e.samples, old_starts = e.starts;
    engine_sample(&e, e.now, 1);
    assert(e.fault == F_TIME && e.samples == old_samples);
    assert(e.starts == old_starts && e.aborted == 2 && !e.active);
    hold(&e, 0, 10);
    assert(e.samples == old_samples && engine_valid(&e));
    engine_init(&e);
    e.samples = UINT32_MAX - 1; e.unknown = UINT32_MAX - 1;
    hold(&e, 0, 1);
    assert(!e.fault && e.samples == UINT32_MAX);
    hold(&e, 0, 1);
    assert(e.fault == F_OVERFLOW);
    engine_init(&e);
    hold(&e, 1, 10);
    char line[1024];
    size_t n = wire_encode(&e, line, sizeof line);
    assert(n && line[n-1] == '\n');
    assert(!wire_encode(&e, line, 10));
    assert(wire_crc("123456789", 9) == 0x29b1);
    ++e.same;
    assert(!engine_valid(&e) && !wire_encode(&e, line, sizeof line));
    puts("PASS: 4096 word triples, parity oracle, episodes, glitches, gap, wrap, halt, overflow, CRC");
}
