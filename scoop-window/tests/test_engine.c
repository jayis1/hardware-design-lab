/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static uint32_t tick;
static void hold(Engine *e, unsigned mask, unsigned count)
{
    while (count--) {
        engine_sample(e, ++tick, (uint8_t)mask);
        assert(engine_valid(e));
    }
}
static Engine baseline(void)
{
    Engine e;
    engine_init(&e);
    hold(&e, 15, 10);
    assert(e.qualified == 15 && e.ready == 3);
    return e;
}
int main(int argc, char **argv)
{
    if (argc > 1 && argv[1][0] == 't') {
        Engine trace = baseline();
        uint32_t seed = 42;
        unsigned mask = 15;
        for (unsigned n = 0; n < 2000; ++n) {
            if (n % 9 == 0) {
                seed = seed * 1664525u + 1013904223u;
                mask = (seed >> 16) & 15u;
            }
            if (n % 97 == 0) tick += 5;
            hold(&trace, mask, 1);
            char packet[512];
            assert(wire_encode(&trace, packet, sizeof packet));
            fputs(packet, stdout);
        }
        return 0;
    }
    Engine e = baseline();
    /* A scoop leaves; B closed, then B opens, then A returns. */
    hold(&e, 14, 10);
    assert(e.active == 1 && e.starts[0] == 1);
    hold(&e, 6, 20);
    assert(e.flagged[0] == 1 && e.foreign_ms[0] > 0);
    hold(&e, 7, 10);
    assert(e.completed[0] == 1 && !e.active);
    uint32_t retained = e.foreign_ms[0];
    hold(&e, 15, 10);
    assert(e.foreign_ms[0] == retained);
    assert(e.starts[1] == 0);
    char frame[512];
    assert(wire_encode(&e, frame, sizeof frame));
    if (argc > 1) { fputs(frame, stdout); return 0; }
    assert(!wire_encode(&e, frame, 8));
    assert(wire_crc("123456789",9) == 0x29b1);
    /* Symmetric B case, both tools absent and both lids open. */
    e = baseline();
    hold(&e, 0, 30);
    assert(e.active == 3 && e.flagged[0] == 1 && e.flagged[1] == 1);
    hold(&e, 15, 10);
    assert(e.completed[0] == 1 && e.completed[1] == 1);
    /* Startup absent is not an observed departure. */
    engine_init(&e);
    hold(&e, 0, 30);
    assert(!e.active && !e.starts[0] && !e.starts[1]);
    hold(&e, 15, 10);
    hold(&e, 14, 10);
    assert(e.starts[0] == 1);
    /* Lost sampling aborts, preserves totals, and requires redocking. */
    tick += 100;
    engine_sample(&e, tick, 14);
    assert(e.aborted[0] == 1 && e.gaps == 1 && !e.active);
    hold(&e, 14, 30);
    assert(e.starts[0] == 1 && !(e.ready & 1));
    hold(&e, 15, 10);
    hold(&e, 14, 10);
    assert(e.starts[0] == 2 && e.active == 1);
    /* A chatter on its own lid must not hide foreign B-open evidence. */
    e = baseline();
    hold(&e, 14, 10);
    for (unsigned i=0; i<100; ++i) hold(&e, (i&1) ? 6 : 4, 1);
    assert(e.flagged[0] == 1 && e.foreign_ms[0] >= 90);
    /* Short raw glitches do not create a qualified departure. */
    e = baseline();
    hold(&e, 14, 4);
    hold(&e, 15, 10);
    assert(!e.starts[0]);
    /* Fifth observation accepts departure; repeated foreign openings mark once. */
    hold(&e, 14, 5);
    assert(e.starts[0] == 1 && e.active == 1);
    hold(&e, 6, 10); hold(&e, 14, 10); hold(&e, 6, 10);
    assert(e.flagged[0] == 1);
    /* Return tied to foreign opening has no demonstrable open/away endpoint. */
    e = baseline(); hold(&e, 14, 10); hold(&e, 7, 5);
    assert(e.completed[0] == 1 && e.flagged[0] == 0);
    e = baseline();
    /* Equality and ordinary uint32 tick wrapping remain valid. */
    uint32_t before = e.now;
    engine_sample(&e, before, 0);
    assert(e.now == before && !e.starts[0]);
    tick=UINT32_MAX-20;
    e = baseline();
    hold(&e, 14, 30);
    assert(e.gaps == 0 && e.active == 1);
    /* Saturation must roll back a whole tick before abandoning activity. */
    e = baseline();
    hold(&e, 14, 10);
    e.away_ms[0]=UINT32_MAX;
    hold(&e, 14, 1);
    assert(e.faults == F_OVERFLOW && !e.active && e.aborted[0] == 1);
    assert(wire_encode(&e, frame, sizeof frame));
    uint32_t away=e.away_ms[0];
    hold(&e, 0, 20);
    assert(e.away_ms[0] == away);
    /* Reject impossible public snapshots even if individually in range. */
    e = baseline(); e.foreign_ms[0]=1;
    assert(!engine_valid(&e) && !wire_encode(&e,frame,sizeof frame));
    e = baseline(); hold(&e, 14, 10); e.flagged[0]=1;
    assert(!engine_valid(&e));
    engine_init(&e); engine_fault(&e,F_WATCHDOG);
    assert(engine_valid(&e) && wire_encode(&e,frame,sizeof frame));
    /* All input transitions: invariants and encoder agree each sample. */
    for (unsigned from=0; from<16; ++from) {
        for (unsigned to=0; to<16; ++to) {
            e=baseline(); hold(&e,from,8); hold(&e,to,8);
            assert(wire_encode(&e,frame,sizeof frame));
        }
    }
    puts("PASS engine: symmetric custody, gaps, qualification, wrap, overflow, 256 transitions, CRC");
}
