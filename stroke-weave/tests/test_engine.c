/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, raw);
        assert(engine_valid(e));
    }
}
static Engine ready(void)
{
    Engine e;
    engine_init(&e);
    hold(&e, 5, 5);
    assert(e.lane[0].phase == HOME && e.lane[1].phase == HOME);
    return e;
}
static void cycle(Engine *e, unsigned lane)
{
    unsigned other = lane ? 1 : 4;
    hold(e, other, 5);
    hold(e, other | (lane ? 8 : 2), 5);
    hold(e, other, 5);
    hold(e, 5, 5);
}
int main(void)
{
    Engine e = ready();
    cycle(&e, 0); cycle(&e, 1); cycle(&e, 1);
    assert(e.lane[0].deliveries == 1 && e.lane[1].deliveries == 2);
    assert(e.alternating == 1 && e.repeated == 1 && e.peak == 2);
    assert(e.lane[0].refills == 1 && e.lane[1].refills == 2);
    /* Every short delivery word against an independent last-letter oracle. */
    for (unsigned word = 0; word < 256; ++word) {
        e = ready();
        unsigned count[2] = {0}, same = 0, alt = 0, last = 2, streak = 0, peak = 0;
        for (unsigned j = 0; j < 8; ++j) {
            unsigned side = (word >> j) & 1;
            cycle(&e, side); ++count[side];
            if (last == side) { ++same; ++streak; }
            else { if (last != 2) ++alt; streak = 1; }
            if (streak > peak) peak = streak;
            last = side;
            assert(e.lane[0].deliveries == count[0]);
            assert(e.lane[1].deliveries == count[1]);
            assert(e.repeated == same && e.alternating == alt && e.peak == peak);
        }
    }
    e = ready(); hold(&e, 0, 5); hold(&e, 10, 5);
    assert(e.ties == 1 && !e.anchor && !e.streak);
    assert(e.lane[0].deliveries == 1 && e.lane[1].deliveries == 1);
    e = ready(); hold(&e, 4, 5); hold(&e, 5, 5);
    assert(e.lane[0].canceled == 1 && !e.lane[0].deliveries);
    e = ready(); hold(&e, 6, 5);
    assert(e.fault == F_BOUNDARY); /* No witnessed clear region. */
    e = ready(); hold(&e, 7, 5);
    assert(e.fault == F_GEOMETRY);
    e = ready(); hold(&e, 4, 5);
    engine_sample(&e, e.now + 2, 4);
    assert(e.fault == F_GAP && e.lane[0].abandoned == 1 && engine_valid(&e));
    e = ready(); hold(&e, 4, 5); hold(&e, 4, DEADLINE - 5);
    hold(&e, 6, 5); assert(e.fault == F_DEADLINE && !e.lane[0].deliveries);
    e = ready(); hold(&e, 4, 5); hold(&e, 4, DEADLINE - 6);
    hold(&e, 6, 5); assert(!e.fault && e.lane[0].deliveries == 1);
    /* Other lane chatter cannot suppress a stable endpoint. */
    e = ready(); hold(&e, 4, 5);
    for (unsigned i = 0; i < 5; ++i) hold(&e, 2 | ((i & 1) ? 4 : 0), 1);
    assert(e.lane[0].deliveries == 1);
    /* Both starts must roll back when either lane exhausts. */
    e = ready(); e.lane[1].starts = LIMIT; e.lane[1].canceled = LIMIT;
    hold(&e, 0, 5);
    assert(e.fault == F_COUNTER && !e.lane[0].starts && engine_valid(&e));
    e = ready(); e.now = 0xfffffffd;
    hold(&e, 5, 5); assert(!e.fault);
    char buffer[768];
    Engine invalid = ready();
    invalid.lane[0].starts = invalid.lane[0].abandoned = 1;
    assert(!engine_valid(&invalid));
    invalid = ready();
    invalid.lane[0].starts = invalid.lane[0].deliveries = 2;
    invalid.lane[0].last_ms = 5;
    invalid.lane[0].phase = END;
    invalid.qualified = invalid.raw = 6;
    invalid.anchor = 1; invalid.streak = invalid.peak = 2; invalid.repeated = 1;
    assert(!engine_valid(&invalid));
    assert(wire_crc("123456789", 9) == 0x29b1);
    assert(wire_encode(&e, buffer, sizeof buffer));
    assert(!wire_encode(&e, buffer, 3));
    e.lane[0].deliveries = 99;
    assert(!wire_encode(&e, buffer, sizeof buffer));
    puts("PASS stroke engine: words, ties, cancellations, faults, deadlines, overflow, framing");
}
