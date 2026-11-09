/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static uint32_t clock_ms;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++clock_ms, (uint8_t)raw);
        assert(engine_valid(e));
        char frame[768];
        assert(wire_encode(e, frame, sizeof frame));
    }
}
static void begin(Engine *e)
{
    engine_init(e);
    clock_ms = 0;
    hold(e, 0, 6);
    hold(e, 4, 6);
    assert(e->state == ACTIVE);
    hold(e, 0, 6);
}
static void turn(Engine *e, unsigned teeth, int direction)
{
    const unsigned positive[] = {1, 3, 2, 0};
    const unsigned negative[] = {2, 3, 1, 0};
    for (unsigned i=0; i<96; ++i) {
        unsigned q = direction > 0 ? positive[i%4] : negative[i%4];
        hold(e, q, 6);
        if (i < teeth) {
            hold(e, q|8, 6);
            hold(e, q, 6);
        }
    }
    hold(e, 4, 6);
}
int main(void)
{
    Engine e;
    for (int direction=-1; direction<=1; direction+=2) {
        for (unsigned teeth=0; teeth<=96; ++teeth) {
            begin(&e);
            turn(&e, teeth, direction);
            assert(e.good==1 && e.bad==0 && e.last==teeth);
            assert(e.reference==teeth && e.minimum==teeth && e.maximum==teeth);
            assert(e.starts==2 && e.state==ACTIVE);
        }
    }
    begin(&e); turn(&e, 12, 1); hold(&e, 0, 6);
    turn(&e, 11, 1);
    assert(e.good==2 && e.mismatch==1 && e.total==23);
    assert(e.minimum==11 && e.maximum==12 && e.reference==12);
    begin(&e); hold(&e, 1, 6); hold(&e, 0, 6);
    assert(e.bad==1 && e.reason==R_REVERSE && e.state==SEEK);
    begin(&e); hold(&e, 3, 6);
    assert(e.bad==1 && e.reason==R_QUADRATURE);
    begin(&e); hold(&e, 4, 6);
    assert(e.bad==1 && e.reason==R_STEPS && e.starts==2);
    begin(&e); hold(&e, 8, 6); hold(&e, 12, 6);
    assert(e.bad==1 && e.reason==R_BOUNDARY && e.state==SEEK);
    begin(&e); hold(&e, 0, TIMEOUT);
    assert(e.bad==1 && e.reason==R_TIMEOUT && e.state==SEEK);
    /* All ordered quadrature transitions against an independent ring oracle. */
    const unsigned ring[]={0,1,3,2};
    for (unsigned a=0;a<4;++a) {
        for (unsigned b=0;b<4;++b) {
            engine_init(&e); clock_ms=0;
            hold(&e,ring[a],6); hold(&e,ring[a]|4,6); hold(&e,ring[a],6);
            hold(&e,ring[b],6);
            unsigned distance=(b+4-a)%4;
            assert(e.bad==(distance==2));
            if (distance!=2) {
                assert(e.steps==(distance!=0));
                assert(e.direction==(distance==1 ? 1u : distance==3 ? 2u : 0u));
            }
        }
    }
    /* Boundary sample at the rejecting deadline cannot complete. */
    for (unsigned deadline=TIMEOUT-1;deadline<=TIMEOUT;++deadline) {
        begin(&e);
        const unsigned q[]={1,3,2,0};
        for (unsigned i=0;i<96;++i) hold(&e,q[i%4],6);
        hold(&e,0,deadline-e.age-5);
        hold(&e,4,5);
        assert(e.good==(deadline<TIMEOUT));
        assert(e.bad==(deadline==TIMEOUT));
        if (deadline==TIMEOUT) assert(e.reason==R_TIMEOUT && e.starts==1);
    }
    /* A short raw tooth glitch on the cut may not be silently accepted. */
    begin(&e);
    const unsigned q[]={1,3,2,0};
    for (unsigned i=0;i<96;++i) hold(&e,q[i%4],6);
    hold(&e,4,4); hold(&e,12,1);
    assert(e.bad==1 && e.reason==R_BOUNDARY && e.good==0);
    begin(&e); engine_sample(&e, clock_ms+2, 0);
    assert(e.state==HALT && e.bad==1 && e.reason==R_GAP);
    assert(engine_valid(&e));
    Engine frozen=e; engine_sample(&e, clock_ms+50, 15);
    assert(e.sequence==frozen.sequence && e.raw==frozen.raw && e.bad==frozen.bad);
    engine_init(&e); clock_ms=0; hold(&e, 4, 20);
    assert(e.starts==0); /* A held index on boot is not a rising passage. */
    hold(&e, 0, 6); hold(&e, 4, 6); assert(e.starts==1);
    /* Independent index qualification must survive tooth chatter. */
    engine_init(&e); clock_ms=0; hold(&e, 0, 6);
    for (unsigned i=0;i<6;++i) hold(&e, 4 | (i%2 ? 8:0), 1);
    assert(e.starts==0); /* Raw tooth at index boundary blocks start. */
    engine_init(&e); clock_ms=0; hold(&e,0,6);
    e.sequence=LIMIT; engine_sample(&e, ++clock_ms, 0);
    assert(e.state==HALT && (e.fault&F_COUNTER));
    assert(engine_valid(&e));
    engine_init(&e); clock_ms=0xfffffffa; hold(&e,0,12);
    assert(e.state==SEEK && !e.fault);
    assert(wire_crc("123456789",9)==0x29b1);
    char small[2]; assert(!wire_encode(&e,small,sizeof small));
    puts("PASS engine: both directions, all 0..96 counts, mismatch, faults, wrap, CRC");
}
