/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned ms)
{
    while (ms--) engine_sample(e, e->time + 1, mask);
    assert(engine_valid(e));
}
static void ready(Engine *e, unsigned side)
{
    engine_init(e);
    hold(e, side | 8, 10);
    assert(e->state == READY && e->anchor == side);
}
static void start(Engine *e)
{
    hold(e, 8, 5);
    assert(e->state == ACTIVE);
}
static void pulse(Engine *e)
{
    hold(e, 12, 6);
    hold(e, 8, 6);
}
int main(int argc, char **argv)
{
    Engine e;
    char line[768];
    assert(wire_crc("123456789", 9) == 0x29b1);
    for (unsigned side=1; side<=2; ++side) {
        for (unsigned count=0; count<40; ++count) {
            ready(&e, side); start(&e);
            for (unsigned n=0; n<count; ++n) pulse(&e);
            hold(&e, (3^side)|8, 5);
            assert(e.completed==1 && e.last_pulses==count && !e.aborted);
            assert(e.pulses==count && e.last_end==(3^side));
            start(&e); pulse(&e);
            hold(&e, side|8, 5);
            assert(e.completed==2 && e.last_pulses==1);
        }
    }
    ready(&e,1); start(&e); pulse(&e);
    hold(&e,9,5);
    assert(e.aborted==1 && e.abandoned==1 && !e.completed);
    ready(&e,1); start(&e);
    hold(&e,0,1); hold(&e,8,5); hold(&e,10,5);
    assert(e.last_bad==1 && e.bad_total==1);
    ready(&e,1); start(&e); hold(&e,12,4); hold(&e,8,5);
    assert(!e.pulses);
    hold(&e,14,5);
    assert(e.aborted==1 && !e.completed);
    ready(&e,1); start(&e); pulse(&e);
    engine_sample(&e,e.time+2,8);
    assert(e.state==HALTED && e.aborted==1 && e.abandoned==1 && e.gaps==1);
    assert(engine_valid(&e));
    hold(&e,10,20); assert(!e.completed);
    ready(&e,1); start(&e); e.active_ms=MAX_TRAVERSE_MS-1;
    hold(&e,8,1); assert(e.aborted==1 && e.state==WAIT);
    ready(&e,1); hold(&e,11,5);
    assert(e.state==HALTED && (e.flags&F_ENDPOINTS));
    ready(&e,1); e.time=UINT32_MAX; start(&e);
    assert(e.state==ACTIVE);
    ready(&e,1); e.starts=UINT32_MAX; e.aborted=UINT32_MAX;
    hold(&e,8,5); assert(e.state==HALTED && engine_valid(&e));
    ready(&e,1); start(&e); e.pulses=UINT32_MAX; e.abandoned=UINT32_MAX;
    hold(&e,12,5); assert(e.state==HALTED && engine_valid(&e));
    ready(&e,1); start(&e); pulse(&e); hold(&e,10,5);
    assert(wire_encode(&e,line,sizeof line));
    assert(!wire_encode(&e,line,8));
    e.last_pulses=2; assert(!engine_valid(&e));
    /* Exhaust all short raw observation sequences against invariants. */
    for (unsigned word=0; word<65536; ++word) {
        ready(&e,1);
        for (unsigned shift=0; shift<16; shift+=4)
            hold(&e,(word>>shift)&15,5);
    }
    if (argc>1 && !strcmp(argv[1],"--fixture")) {
        ready(&e,1);
        for (unsigned layer=0; layer<4; ++layer) {
            start(&e);
            for (unsigned n=0; n<=layer; ++n) pulse(&e);
            hold(&e,(layer%2 ? 1 : 2)|8,5);
            wire_encode(&e,line,sizeof line); fputs(line,stdout);
        }
    } else puts("PASS engine boundaries, mirrored traverses, exhaustive raw sequences and CRC");
    return 0;
}
