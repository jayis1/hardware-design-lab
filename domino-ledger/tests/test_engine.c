/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned ticks)
{
    while (ticks--) engine_sample(e, e->now + 1u, (uint8_t)raw);
}
static void ready(Engine *e)
{
    engine_init(e);
    hold(e, 15, 505);
    assert(e->phase == READY);
}
static void permutation(unsigned a, unsigned b, unsigned c, unsigned d)
{
    Engine e;
    ready(&e);
    unsigned order[] = {a,b,c,d}, mask = 15;
    for (unsigned i=0; i<4; ++i) {
        mask &= ~(1u << order[i]);
        hold(&e, mask, 10);
        assert(e.offset[order[i]] == 10*i);
    }
    hold(&e, 0, 250);
    assert(e.completed == 1 && e.full == 1 && e.partial == 0);
    assert(e.seen == 15 && e.rebound == 0 && e.outcome == CLEAN_FULL);
    char frame[768];
    assert(wire_encode(&e, frame, sizeof frame));
    assert(!wire_encode(&e, frame, 8));
    assert(engine_valid(&e));
}
int main(void)
{
    for (unsigned a=0;a<4;++a) for (unsigned b=0;b<4;++b)
    for (unsigned c=0;c<4;++c) for (unsigned d=0;d<4;++d)
        if(a!=b && a!=c && a!=d && b!=c && b!=d && c!=d) permutation(a,b,c,d);
    Engine e;
    ready(&e);
    hold(&e, 0, 5);
    assert(e.seen == 15);
    for(unsigned i=0;i<4;++i) assert(e.offset[i]==0);
    hold(&e, 0, 249);
    assert(e.phase == ACTIVE);
    hold(&e, 0, 1);
    assert(e.phase == DONE && e.full==1);
    /* Rebuild does not rewrite the frozen trial. */
    hold(&e, 15, 505);
    assert(e.phase == READY && e.rebound==0 && e.full==1);
    hold(&e, 14, 5);
    hold(&e, 15, 5);
    assert(e.rebound == 1);
    hold(&e, 0, 5);
    hold(&e, 0, 250);
    assert(e.outcome == DISTURBED_FULL && e.disturbed==1);
    /* Deadline rejects a fall that qualifies exactly at 10000. */
    ready(&e); hold(&e, 14, 5);
    hold(&e, 14, 9995); hold(&e, 0, 5);
    assert(e.age == 10000 && e.seen == 1 && e.partial==1);
    assert(e.offset[1]==ABSENT);
    /* A raw glitch interrupts quiet time despite unchanged qualification. */
    ready(&e); hold(&e, 0, 5); hold(&e, 0, 249);
    hold(&e, 1, 1); hold(&e, 0, 250);
    assert(e.phase==ACTIVE); hold(&e, 0, 1); assert(e.phase==DONE);
    /* Unrelated chatter cannot postpone channel A qualification. */
    ready(&e);
    for(unsigned i=0;i<5;++i) hold(&e, (i&1)?12:14, 1);
    assert(e.seen==1 && e.phase==ACTIVE);
    /* A missed tick abandons correlation, preserving the pending evidence. */
    engine_sample(&e, e.now+2, 0);
    assert(e.phase==HALT && e.aborted==1 && e.outcome==ABORTED);
    assert(e.starts==e.completed+e.aborted);
    assert(engine_valid(&e));
    hold(&e,15,1000); assert(e.phase==HALT);
    /* Modular uptime wrap is not a gap. */
    ready(&e); e.now=0xfffffffeu; hold(&e,15,3);
    assert(e.now==1 && e.phase==READY);
    /* Counter exhaustion rolls back speculative qualification and completion. */
    ready(&e); e.samples=UINT32_MAX;
    hold(&e,14,1); assert(e.fault==F_OVERFLOW && e.starts==0);
    ready(&e); hold(&e,14,5); e.tx_errors=UINT32_MAX;
    engine_tx_error(&e);
    assert(e.phase==HALT && e.aborted==1);
    assert(engine_valid(&e));
    puts("PASS: 24 fall permutations, ties, deadlines, rearm, glitches, gaps, wrap, overflow, wire bounds");
    return 0;
}
