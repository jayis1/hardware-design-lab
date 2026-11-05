/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void ticks(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, raw);
        assert(engine_valid(e));
    }
}
static Engine ready(unsigned origin)
{
    Engine e;
    engine_init(&e);
    ticks(&e, origin, 56);
    assert(e.state == READY && e.origin == origin);
    return e;
}
static void loaded(Engine *e, unsigned origin)
{
    unsigned push = origin == 1 ? 4 : 8;
    unsigned destination = origin == 1 ? 2 : 1;
    ticks(e, origin | push, 5);
    assert(e->state == PUSH);
    ticks(e, push, 10);
    ticks(e, destination | push, 5);
    assert(e->state == LOADED && e->reached_ms != ABSENT);
}
int main(void)
{
    for (unsigned origin = 1; origin <= 2; ++origin) {
        unsigned dest = 3 ^ origin;
        unsigned push = origin == 1 ? 4 : 8;
        Engine e = ready(origin);
        loaded(&e, origin);
        ticks(&e, dest, 5);
        assert(e.state == HOLD && e.hold == 0);
        ticks(&e, dest, HOLD_MS - 1);
        assert(e.state == HOLD);
        ticks(&e, dest, 1);
        assert(e.state == DONE && e.result == RETAINED);
        uint32_t finish = e.finish_ms;
        ticks(&e, 15, 100);
        assert(e.result == RETAINED && e.finish_ms == finish);
        char frame[768];
        assert(wire_encode(&e, frame, sizeof frame) > 0);
        assert(!wire_encode(&e, frame, 2));
        assert(wire_crc("123456789", 9) == 0x29b1);
        e = ready(origin);
        ticks(&e, origin | push, 5);
        ticks(&e, origin, 5);
        assert(e.state == DONE && e.result == EARLY);
        e = ready(origin);
        loaded(&e, origin);
        ticks(&e, dest, 5);
        ticks(&e, dest, 499);
        ticks(&e, 0, 1); /* Raw loss exactly at completion wins. */
        assert(e.result == REBOUND);
        e = ready(origin);
        loaded(&e, origin);
        ticks(&e, dest, 5);
        ticks(&e, dest | push, 1);
        assert(e.result == REBOUND);
        e = ready(origin);
        ticks(&e, origin | push, 5);
        ticks(&e, 15, 1);
        assert(e.result == CONFLICT);
        e = ready(origin);
        ticks(&e, origin | push, 5);
        ticks(&e, origin | push, DEADLINE_MS - 1);
        assert(e.state == PUSH);
        ticks(&e, origin | push, 1);
        assert(e.result == TIMEOUT && e.finish_ms == DEADLINE_MS);
        e = ready(origin);
        loaded(&e, origin);
        engine_sample(&e, e.now + 2, dest);
        assert(e.state == HALT && e.result == ABORT && e.gaps == 1);
        assert(engine_valid(&e));
    }
    /* Held paddle at boot must not start a trial. */
    Engine e;
    engine_init(&e);
    ticks(&e, 5, 100);
    assert(e.state == WAIT && e.origin == 0);
    ticks(&e, 1, 56);
    assert(e.state == READY);
    ticks(&e, 0, 1);
    assert(e.state == WAIT);
    /* Qualification is independent: the other paddle chatters. */
    engine_init(&e);
    for (unsigned i = 0; i < 20; ++i) ticks(&e, 1 | ((i & 1) ? 8 : 0), 1);
    assert((e.valid & 1) && (e.qualified & 1));
    /* Wrap is contiguous; exhaustion cannot manufacture a retained result. */
    e = ready(1);
    e.now = UINT32_MAX - 2;
    loaded(&e, 1);
    assert(!e.fault);
    e.sequence = UINT32_MAX;
    ticks(&e, 6, 1);
    assert(e.state == HALT && e.result == ABORT && (e.fault & F_EXHAUST));
    /* Exhaustively compare the last HOLD sample with a separate contact oracle. */
    for (unsigned origin = 1; origin <= 2; ++origin) {
        for (unsigned raw = 0; raw < 16; ++raw) {
            e = ready(origin);
            loaded(&e, origin);
            ticks(&e, 3 ^ origin, 5 + HOLD_MS - 1);
            unsigned opposite = origin == 1 ? 8 : 4;
            unsigned expected = ((raw & 3) == 3 || (raw & opposite)) ? CONFLICT :
                                raw == (3 ^ origin) ? RETAINED : REBOUND;
            ticks(&e, raw, 1);
            assert(e.result == expected);
        }
    }
    /* Same completion at the rejecting deadline is not retention. */
    e = ready(1);
    loaded(&e, 1);
    ticks(&e, 2, 5);
    ticks(&e, 2, 499);
    e.age = DEADLINE_MS - 1;
    e.released_ms = e.age - e.hold;
    ticks(&e, 2, 1);
    assert(e.result == TIMEOUT && e.hold == 499);
    /* Structurally plausible frames must also describe the claimed phase. */
    e = ready(1);
    e.reached_ms = 0;
    assert(!engine_valid(&e));
    e = ready(1);
    loaded(&e, 1);
    ticks(&e, 2, 505);
    e.released_ms--;
    assert(!engine_valid(&e));
    puts("PASS both directions, hold boundaries, raw rebound, early release, conflict, deadline, gap, wrap, UART framing");
}
