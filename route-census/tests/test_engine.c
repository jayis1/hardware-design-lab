/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static Engine e;
static void hold(unsigned raw, unsigned ms)
{
    while (ms--) engine_sample(&e, e.time + 1u, raw);
}
static void ready(void)
{
    engine_init(&e, 0, 0);
    hold(0, 110);
    assert(e.state == READY);
}
int main(void)
{
    ready();
    hold(1, 10); hold(0, 10); hold(2, 10); hold(6, 10); hold(0, 110);
    assert(e.accepted == 0 && e.rejected == 1 && e.reason == MULTIPLE);
    ready(); hold(1, 10); e.gaps = UINT32_MAX;
    engine_sample(&e, e.time + 5, 0);
    assert(e.state == FAULT && engine_consistent(&e));
    ready(); e.orphans = UINT32_MAX; hold(2, 10); hold(0, 110);
    assert(e.state == FAULT);
    ready();
    hold(1, 10); hold(0, 50); hold(2, 10); hold(0, 110);
    assert(e.accepted == 1 && e.route[0].count == 1);
    assert(e.last_ms == 60 && e.last_route == 1);
    hold(0, 110); assert(e.state == READY);
    hold(1, 10); hold(0, 20); hold(1, 10);
    assert(e.rejected == 1 && e.reason == OVERLAP);
    ready(); hold(3, 10);
    assert(e.rejected == 1 && e.reason == SIMULTANEOUS);
    ready(); hold(2, 10);
    assert(e.orphans == 1 && e.state == DRAIN);
    ready(); hold(1, 10); hold(0, 30010);
    assert(e.rejected == 1 && e.reason == EXPIRED);
    ready(); hold(1, 10);
    engine_sample(&e, e.time + 4, 0);
    assert(e.gaps == 1 && e.rejected == 1 && e.reason == GAP);
    hold(0, 110); assert(e.state == READY);
    ready(); hold(1, 2); hold(0, 10);
    assert(e.entries == 0); /* pulse below qualification */
    ready(); hold(1, 10); hold(0, 10); hold(6, 10);
    assert(e.reason == MULTIPLE && e.accepted == 0);
    ready(); hold(1, 10); hold(0, 10); hold(8, 10); hold(0, 110);
    assert(e.route[2].count == 1);
    char out[512];
    size_t n = wire_encode(&e, 1, 0, out, sizeof out);
    assert(n > 20 && out[n-1] == '\n');
    assert(wire_encode(&e, 1, 0, out, 8) == 0);
    assert(wire_crc("123456789", 9) == 0x29b1);
    engine_init(&e, UINT32_MAX-50u, 0); hold(0, 110);
    assert(e.state == READY);
    hold(1, 10); hold(0, 20); hold(4, 10); hold(0, 110);
    assert(e.accepted == 1 && e.last_ms == 30);
    /* Independent qualification: exit chatter cannot delay inlet. */
    ready();
    for (unsigned i=0; i<10; ++i) hold(1 | ((i&1)?2:0), 1);
    assert(e.entries == 1 && e.state == FLIGHT);
    ready(); hold(1, 10); hold(0, 30); hold(2, 10); hold(0, 110);
    n=wire_encode(&e, 2, 0, out, sizeof out);
    FILE *f=fopen("build/fixture.txt", "w"); assert(f);
    assert(fwrite(out,1,n,f)==n); fclose(f);
    /* Deterministic noisy-input stress with an independent conservation check. */
    ready();
    uint32_t random = 0x12345678;
    for (unsigned i=0; i<200000; ++i) {
        random = random * 1664525u + 1013904223u;
        uint8_t raw = (random >> 28) & 15u;
        engine_sample(&e, e.time + (i % 997 == 0 ? 5u : 1u), raw);
        assert(engine_consistent(&e));
    }
    /* Worst-width numeric serialization remains within the target buffer. */
    char maximum[640];
    engine_init(&e, UINT32_MAX, 0);
    e.entries=e.accepted=e.rejected=e.orphans=e.gaps=e.block_events=UINT32_MAX;
    e.last_route=e.last_ms=e.fault_mask=UINT32_MAX;
    for (unsigned i=0;i<3;++i) {
        e.route[i].count=e.route[i].minimum=e.route[i].maximum=UINT32_MAX;
        e.route[i].sum=UINT64_MAX;
        for(unsigned j=0;j<6;++j) e.route[i].bins[j]=UINT32_MAX;
    }
    assert(wire_encode(&e, UINT32_MAX, UINT32_MAX, maximum, sizeof maximum)>0);
    puts("PASS route state machine, gaps, debounce, wrap, framing");
}
