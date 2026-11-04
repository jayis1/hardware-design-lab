/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void step(Engine *e, unsigned count, unsigned mask)
{
    while (count--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
    }
}
static Engine armed(void)
{
    Engine e;
    engine_init(&e);
    step(&e, 300, 0);
    assert(e.phase == READY);
    return e;
}
int main(void)
{
    Engine startup;
    engine_init(&startup);
    step(&startup, 400, 15);
    assert(startup.phase == WAIT && startup.starts == 0);
    step(&startup, 254, 7);
    assert(startup.phase == WAIT);
    step(&startup, 1, 7);
    assert(startup.phase == READY);
    /* RECORD qualification cannot be postponed by other input chatter. */
    for (unsigned i = 0; i < 5; ++i)
        step(&startup, 1, 8 | (i & 1));
    assert(startup.phase == ACTIVE);
    /* Four samples cannot qualify a new contact; the fifth does. */
    for (unsigned bit = 0; bit < 3; ++bit) {
        step(&startup, 10, 8);
        step(&startup, 4, 8 | (1u << bit));
        assert(!(startup.mask & (1u << bit)));
        step(&startup, 1, 8 | (1u << bit));
        assert(startup.mask & (1u << bit));
    }
    Engine e = armed();
    step(&e, 5, 8);
    assert(e.phase == ACTIVE && e.age == 0);
    step(&e, 100, 8);
    assert(e.hist[0] == 100 && e.runs[0] == 1 && e.longest[0] == 100);
    step(&e, 1, 9); /* Rejected raw glitch must break observed continuity. */
    step(&e, 10, 8);
    assert(e.hist[0] == 109 && e.unknown == 2);
    assert(e.runs[0] == 2 && e.longest[0] == 100);
    step(&e, 5, 0);
    assert(e.phase == DONE && e.completed == 1 && e.age == 116);
    Engine frozen = e;
    step(&e, 100, 7);
    for (unsigned i = 0; i < 8; ++i) assert(e.hist[i] == frozen.hist[i]);
    step(&e, 200, 7);
    assert(e.phase == READY);
    step(&e, 5, 15);
    assert(e.phase == ACTIVE && e.starts == 2 && e.age == 0);
    step(&e, 10, 15);
    assert(e.hist[7] == 10);
    engine_sample(&e, e.now + 2, 15);
    assert(e.phase == HALT && e.aborted == 1 && e.fault == F_GAP);
    assert(engine_valid(&e));
    char buffer[768];
    assert(wire_encode(&e, buffer, sizeof buffer));
    assert(!wire_encode(&e, buffer, 1));
    e = armed();
    step(&e, 5, 8);
    step(&e, DEADLINE_MS - 1, 8);
    assert(e.phase == ACTIVE);
    step(&e, 1, 0);
    assert(e.phase == HALT && e.fault == F_TIMEOUT && e.age == DEADLINE_MS);
    e = armed();
    e.starts = e.completed = UINT32_MAX;
    step(&e, 5, 8);
    assert(e.phase == HALT && e.fault == F_OVERFLOW && e.starts == UINT32_MAX);
    assert(engine_valid(&e));
    e = armed();
    e.now = UINT32_MAX - 3;
    step(&e, 10, 0);
    assert(e.phase == READY);
    /* Independent run-length oracle: all four-word walks over 8 zones. */
    for (unsigned word = 0; word < 4096; ++word) {
        e = armed();
        step(&e, 5, 8);
        unsigned code = word;
        unsigned bins[8] = {0}, runs[8] = {0}, maxima[8] = {0};
        unsigned streak = 0, previous = 99, unknown = 0;
        for (unsigned j = 0; j < 4; ++j) {
            unsigned zone = code & 7; code >>= 3;
            for (unsigned k = 0; k < 12; ++k) {
                unsigned oldraw = e.raw, oldmask = e.mask;
                step(&e, 1, zone | 8);
                /* Oracle uses the independently qualified endpoints only. */
                if (oldraw == oldmask && e.raw == e.mask && oldmask == e.mask) {
                    ++bins[zone];
                    if (previous != zone) { ++runs[zone]; streak = 0; }
                    ++streak;
                    if (streak > maxima[zone]) maxima[zone] = streak;
                    previous = zone;
                } else { ++unknown; previous = 99; streak = 0; }
            }
        }
        for (unsigned i = 0; i < 8; ++i) {
            assert(e.hist[i] == bins[i]);
            assert(e.runs[i] == runs[i]);
            assert(e.longest[i] == maxima[i]);
        }
        assert(e.unknown == unknown);
    }
    puts("PASS trial accounting, glitch continuity, timeout, overflow, wrap and 4096 occupancy walks");
}
