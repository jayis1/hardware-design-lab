/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Engine e;
static uint32_t tick;
static void hold(unsigned mask, unsigned duration)
{
    while (duration--) {
        engine_sample(&e, ++tick, (uint8_t)mask);
        assert(engine_valid(&e));
    }
}
static void ready(void)
{
    tick = 0;
    engine_init(&e);
    hold(0, 255);
    assert(e.phase == READY);
}
static void words(unsigned depth, unsigned word)
{
    ready();
    unsigned previous = 0, n = 0;
    for (unsigned i = 0; i < depth; ++i) {
        unsigned mask = (word >> (4*i)) & 15u;
        hold(mask, 5);
        if (mask != previous) {
            assert(e.event[n].mask == mask);
            ++n;
        }
        previous = mask;
        assert(e.count == n);
    }
}
int main(void)
{
    engine_init(&e); tick=0;
    hold(15, 1000);
    assert(e.phase == WAIT && e.count == 0);
    ready();
    hold(1,5); hold(3,5); hold(2,5); hold(10,5); hold(15,5);
    assert(e.count == 5 && e.event[2].mask == 2);
    assert(e.event[0].at == 0 && e.event[4].at == 20);
    hold(15,249); assert(e.phase == ACTIVE);
    hold(15,1); assert(e.phase == DONE);
    Engine frozen=e;
    hold(0,1000);
    assert(e.age == frozen.age && e.count == frozen.count);
    assert(!memcmp(e.event,frozen.event,sizeof e.event));
    /* A raw glitch cancels settlement even if it never qualifies. */
    ready(); hold(15,5); hold(15,249); hold(14,1); hold(15,249);
    assert(e.phase == ACTIVE); hold(15,2); assert(e.phase == DONE);
    /* Chatter on D does not postpone A's qualification. */
    ready();
    for (unsigned i=0;i<5;++i) hold(1 | ((i&1)?8:0),1);
    assert(e.count == 1 && e.event[0].mask == 1);
    /* Capture every short word, including arbitrary reopenings and ties. */
    for (unsigned w=0;w<65536;++w) words(4,w);
    ready(); hold(1,5);
    engine_sample(&e,tick+2,1);
    assert(e.phase == HALT && e.fault == F_GAP && engine_valid(&e));
    ready(); hold(1,5); hold(1,DEADLINE_MS-1);
    assert(e.phase == ACTIVE); hold(1,1);
    assert(e.phase == HALT && e.fault == F_TIMEOUT);
    /* Completion exactly at the rejecting deadline must remain a timeout. */
    ready(); hold(1,5); hold(1,DEADLINE_MS-255);
    hold(15,5); hold(15,249);
    assert(e.age == DEADLINE_MS-1 && e.quiet == 249);
    hold(15,1); assert(e.phase == HALT && e.fault == F_TIMEOUT);
    Engine impossible=e;
    impossible.phase=DONE; impossible.fault=F_NONE; impossible.quiet=250;
    assert(!engine_valid(&impossible));
    ready();
    for (unsigned i=0;i<MAX_EVENTS;++i) hold((i&1)?2:1,5);
    assert(e.count == MAX_EVENTS && e.phase == ACTIVE);
    hold(4,5); assert(e.phase == HALT && e.fault == F_CAPACITY);
    assert(e.count == MAX_EVENTS);
    ready(); e.samples=UINT32_MAX; hold(0,1);
    assert(e.phase == HALT && e.fault == F_OVERFLOW);
    char frame[768];
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,4)==0);
    Engine bad=e; bad.event[0].at=1; assert(!engine_valid(&bad));
    ready(); tick=UINT32_MAX-2; e.now=tick; hold(0,5);
    assert(e.phase == READY);
    puts("PASS: exhaustive 65536 mask words, ties, reopenings, limits, gaps, wrap, raw glitch and encoder");
}
