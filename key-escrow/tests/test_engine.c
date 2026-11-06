/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
        char frame[512];
        assert(wire_encode(e, frame, sizeof frame));
    }
}
static Engine ready(void)
{
    Engine e;
    engine_init(&e);
    hold(&e, 10, 60);
    assert(e.state == READY);
    return e;
}
static void access(Engine *e)
{
    hold(e, 14, 5); /* key first, while bolt extended */
    assert(e->state == ACTIVE);
    hold(e, 12, 5); /* bolt between endpoints */
    hold(e, 13, 5); /* fully retracted before opening */
    hold(e, 5, 5);  /* flap open */
    assert(e->opened);
}
static void returned(Engine *e)
{
    hold(e, 13, 5);
    hold(e, 12, 5);
    hold(e, 14, 5);
    hold(e, 10, 5);
}
int main(void)
{
    Engine e = ready();
    access(&e); returned(&e);
    assert(e.completed == 1 && e.aborted == 0 && e.state == WAIT);
    hold(&e, 10, 51);
    hold(&e, 14, 5); hold(&e, 10, 5);
    assert(e.canceled == 1 && e.completed == 1);
    e = ready(); access(&e); hold(&e, 1, 5);
    assert(e.state == HALT && e.result == EARLY_KEY && e.aborted == 1);
    e = ready(); hold(&e, 14, 5); hold(&e, 6, 5);
    assert(e.state == HALT && e.result == BAD_OPEN);
    e = ready(); hold(&e, 14, 5); hold(&e, 12, 5); hold(&e, 5, 5);
    assert(e.result == BAD_OPEN); /* tied retraction/opening lacks prior witness */
    e = ready(); hold(&e, 14, 5); hold(&e, 15, 5);
    assert(e.result == CONTRADICTORY);
    e = ready(); hold(&e, 8, 5);
    assert(e.result == UNKEYED && e.starts == 0);
    e = ready(); access(&e); hold(&e, 6, 5);
    assert(e.result == OPEN_EXTEND);
    e = ready(); access(&e); hold(&e, 14, 5);
    assert(e.result == OPEN_EXTEND); /* closing and extension cannot prove order */
    e = ready(); hold(&e, 14, 5); hold(&e, 12, 5); hold(&e, 10, 5);
    assert(e.result == EARLY_KEY); /* extension tied to key removal */
    e = ready(); hold(&e, 14, 5);
    hold(&e, 14, DEADLINE_MS - 1);
    assert(e.state == ACTIVE && e.age == DEADLINE_MS - 1);
    hold(&e, 10, 1);
    assert(e.result == TIMEOUT && e.aborted == 1);
    e = ready(); access(&e);
    engine_sample(&e, e.now + 2, 5);
    assert(e.state == HALT && e.result == LOST && e.gaps == 1 && engine_valid(&e));
    e = ready(); hold(&e, 14, 5);
    e.sequence = UINT32_MAX;
    hold(&e, 14, 1);
    assert(e.state == HALT && e.aborted == 1 && e.result == EXHAUSTED);
    /* Exact deadline: a qualifying release one tick before is counted;
       the same release at equality is rejected, not merely a later timeout. */
    for (unsigned at = DEADLINE_MS - 1; at <= DEADLINE_MS; ++at) {
        e = ready(); hold(&e, 14, 5);
        hold(&e, 14, at - 5); hold(&e, 10, 5);
        assert(e.age == at);
        assert(e.result == (at == DEADLINE_MS ? TIMEOUT : RETURNED));
    }
    /* Independent truth tables for all outgoing stable masks at the four
       admissible active positions. -1 means remain active, 2 means return. */
    const int table[4][16] = {
        {3,3,3,5,4,4,4,5,3,3,2,5,-1,-1,-1,5}, /* extended, closed */
        {3,3,3,5,4,4,4,5,3,3,3,5,-1,-1,-1,5}, /* between, closed */
        {3,3,3,5,4,-1,4,5,3,3,3,5,-1,-1,-1,5}, /* retracted, closed */
        {3,3,3,5,4,-1,7,5,3,3,3,5,-1,-1,7,5} /* retracted, open */
    };
    for (unsigned position = 0; position < 4; ++position) {
        for (unsigned mask = 0; mask < 16; ++mask) {
            e = ready(); hold(&e, 14, 5);
            if (position >= 1) hold(&e, 12, 5);
            if (position >= 2) hold(&e, 13, 5);
            if (position >= 3) hold(&e, 5, 5);
            hold(&e, mask, 5);
            int expected = table[position][mask];
            if (expected == -1) assert(e.state == ACTIVE);
            else assert(e.result == expected);
        }
    }
    /* Unrelated endpoint chatter must not postpone key-removal qualification. */
    e = ready(); access(&e);
    for (unsigned i = 0; i < 5; ++i) hold(&e, 1 | ((i & 1) ? 2 : 0), 1);
    assert(e.result == EARLY_KEY);
    e = ready(); hold(&e, 14, 5); hold(&e, 12, 5); hold(&e, 13, 5);
    hold(&e, 5, 4); hold(&e, 1, 1); /* raw key loss on opening sample */
    assert(e.result == BAD_OPEN);
    e = ready(); e.now = UINT32_MAX - 2;
    hold(&e, 10, 6); assert(e.state == READY);
    /* Exhaust all stable vectors from armed and opened states. Every invalid
       vector must either halt or stay in a state with defensible invariants. */
    for (unsigned m = 0; m < 16; ++m) {
        e = ready(); hold(&e, m, 6);
        assert(engine_valid(&e));
        e = ready(); access(&e); hold(&e, m, 6);
        assert(engine_valid(&e));
    }
    /* Every short word of qualified vectors, not only nominal paths. */
    for (unsigned word = 0; word < 65536; ++word) {
        e = ready();
        for (unsigned j = 0; j < 4; ++j)
            hold(&e, (word >> (4*j)) & 15, 5);
    }
    e = ready();
    char tiny[3];
    assert(!wire_encode(&e, tiny, sizeof tiny));
    assert(wire_crc("123456789", 9) == 0x29b1);
    puts("PASS engine: cycles, cancellation, faults, deadlines, wrap, exhaustion, 65536 words");
}
