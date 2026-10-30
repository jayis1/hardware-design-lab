/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, unsigned raw, unsigned ms)
{
    while (ms--) {
        engine_sample(e, ++tick, (uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void boot(Engine *e)
{
    tick = 0;
    engine_init(e);
    hold(e, 0, 120);
    assert(e->state == ARMED);
}
static void visit(Engine *e, unsigned gate)
{
    hold(e, 1u << gate, 20);
    hold(e, 0, 120);
    assert(e->state == ARMED);
}
int main(int argc, char **argv)
{
    Engine e;
    boot(&e);
    visit(&e, 0); visit(&e, 1); visit(&e, 2); visit(&e, 1);
    assert(e.visits == 4 && e.loops == 1 && e.erased == 1);
    assert(e.depth == 2 && e.path[0] == 0 && e.path[1] == 1);
    visit(&e, 1);
    assert(e.repeats == 1 && e.loops == 1 && e.visits == 5);
    visit(&e, 3);
    assert(e.depth == 3 && e.path[2] == 3);
    char frame[768];
    assert(wire_encode(&e, frame, sizeof frame));
    if (argc > 1 && !strcmp(argv[1], "--fixture")) {
        fputs(frame, stdout);
        hold(&e, 0, 100);
        assert(wire_encode(&e, frame, sizeof frame));
        fputs(frame, stdout);
        return 0;
    }
    assert(wire_crc("123456789", 9) == 0x29b1);
    assert(!wire_encode(&e, frame, 4));
    if (argc > 1 && !strcmp(argv[1], "--fault-fixtures")) {
        for (unsigned scenario = 0; scenario < 4; ++scenario) {
            boot(&e); visit(&e,0);
            if (scenario == 0) hold(&e,3,20);
            if (scenario == 1) { tick += 6; engine_sample(&e,tick,0); }
            if (scenario == 2) hold(&e,2,30020);
            if (scenario == 3) {
                e.visits=COUNT_LIMIT; e.repeats=COUNT_LIMIT-1;
                hold(&e,1,20); hold(&e,0,120);
            }
            assert(e.state == HALT && wire_encode(&e,frame,sizeof frame));
            fputs(frame,stdout);
        }
        return 0;
    }
    /* Exhaustive independent loop-erasure oracle for all length-eight walks. */
    for (unsigned code = 0; code < 65536; ++code) {
        boot(&e);
        unsigned walk = code, route[4], depth = 0, loops = 0, erased = 0, repeats = 0;
        for (unsigned n = 0; n < 8; ++n) {
            unsigned gate = walk & 3u; walk >>= 2;
            unsigned found = 0;
            while (found < depth && route[found] != gate) ++found;
            if (found == depth) route[depth++] = gate;
            else if (found + 1 == depth) ++repeats;
            else { ++loops; erased += depth - found - 1; depth = found + 1; }
            visit(&e, gate);
            assert(e.depth == depth && e.loops == loops && e.erased == erased && e.repeats == repeats);
            for (unsigned i = 0; i < depth; ++i) assert(e.path[i] == route[i]);
        }
    }
    /* Raw overlap invalidates even if the second contact never qualifies. */
    boot(&e); hold(&e, 1, 20); hold(&e, 3, 1);
    assert(e.state == HALT && e.fault == F_AMBIGUOUS);
    assert(e.visits == 0);
    boot(&e); hold(&e, 1, 20); hold(&e, 0, 5); hold(&e, 2, 20);
    assert(e.state == HALT && e.fault == F_AMBIGUOUS);
    boot(&e); hold(&e, 1, 20); hold(&e, 0, 106);
    assert(e.visits == 1);
    boot(&e); hold(&e, 1, 4); hold(&e, 0, 120);
    assert(!e.visits);
    boot(&e); tick += 6; engine_sample(&e, tick, 0);
    assert(e.state == HALT && e.gaps == 1 && e.fault == F_GAP);
    assert(engine_valid(&e));
    boot(&e); hold(&e, 1, 30020);
    assert(e.state == HALT && e.fault == F_TIMEOUT && !e.visits);
    boot(&e); e.visits = COUNT_LIMIT; e.added = 1;
    e.depth = 1; e.path[0] = 0; e.repeats = COUNT_LIMIT - 1;
    assert(engine_valid(&e));
    hold(&e, 1, 20); hold(&e, 0, 120);
    assert(e.state == HALT && e.fault == F_COUNTER && e.visits == COUNT_LIMIT);
    e.tx_errors = COUNT_LIMIT;
    engine_tx_error(&e);
    assert(e.fault == F_COUNTER);
    boot(&e); engine_fault(&e, F_AMBIGUOUS);
    e.tx_errors = COUNT_LIMIT; engine_tx_error(&e);
    assert(e.fault == F_AMBIGUOUS);
    /* Exact clear boundary, raw clear glitch, maximum allowed sample gap. */
    boot(&e); hold(&e, 1, 20); hold(&e, 0, 105);
    assert(e.clear_ms == 99 && !e.visits);
    hold(&e, 0, 1); assert(e.visits == 1);
    boot(&e); hold(&e, 1, 20); hold(&e, 0, 104);
    hold(&e, 1, 1); hold(&e, 0, 100);
    assert(!e.visits); hold(&e, 0, 1); assert(e.visits == 1);
    boot(&e); tick += 5; engine_sample(&e, tick, 0);
    assert(e.state == ARMED);
    engine_init(&e); tick = UINT32_MAX - 60; hold(&e,0,120);
    visit(&e,2); assert(e.visits == 1 && e.path[0] == 2);
    boot(&e); hold(&e,1,20);
    while (e.elapsed < TIMEOUT_MS - 106) hold(&e,1,1);
    hold(&e,0,106);
    assert(e.state == HALT && e.fault == F_TIMEOUT && !e.visits);
    puts("PASS exhaustive walks, ambiguity, debounce, gap, timeout and capacity");
    return 0;
}
