/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++tick, raw);
        assert(engine_valid(e));
    }
}
static void ready(Engine *e)
{
    tick = 0;
    engine_init(e);
    hold(e, 0, QUALIFY + REARM);
    assert(e->state == ACTIVE && e->starts == 1);
}
static const unsigned ring[4] = {0,1,3,2};
int main(void)
{
    /* Independent ring oracle for every before/after quadrature pair. */
    for (unsigned a=0; a<4; a++) {
        for (unsigned b=0; b<4; b++) {
            int want = b==a ? 0 : b==(a+1)%4 ? 1 : b==(a+3)%4 ? -1 : 2;
            assert(quadrature(ring[a],ring[b]) == want);
        }
    }
    Engine e;
    ready(&e);
    for (unsigned i=1; i<=96; i++)
        hold(&e, ring[i%4] | (ring[i%4]<<2), QUALIFY);
    assert(e.good==1 && e.last==96 && e.total==96 && e.same==1);
    assert(e.starts==2 && e.steps==0 && e.output==0 && e.state==ACTIVE);
    for (unsigned i=1; i<=96; i++)
        hold(&e, ring[(4-i%4)%4] | (ring[i%4]<<2), QUALIFY);
    assert(e.good==2 && e.opposite==1 && e.last==96);
    ready(&e);
    for (unsigned i=1; i<=96; i++)
        hold(&e, ring[i%4], QUALIFY);
    assert(e.good==1 && e.last==0 && e.still==1);
    ready(&e);
    hold(&e,1,QUALIFY); hold(&e,0,QUALIFY);
    assert(e.bad==1 && e.reason==R_REVERSE && e.state==WAIT);
    hold(&e,0,REARM+1); assert(e.state==ACTIVE);
    hold(&e,12,QUALIFY);
    assert(e.bad==2 && e.reason==R_JUMP);
    ready(&e);
    hold(&e,4,QUALIFY); hold(&e,0,QUALIFY);
    assert(e.reason==R_REVERSE && e.bad==1);
    ready(&e);
    hold(&e,0,DEADLINE-1); assert(e.state==ACTIVE);
    hold(&e,0,1); assert(e.bad==1 && e.reason==R_TIMEOUT);
    ready(&e);
    tick+=2; engine_sample(&e,tick,0);
    assert(e.fault==F_TIME && e.state==HALT && e.bad==1);
    uint32_t samples=e.samples;
    hold(&e,15,10); assert(e.samples==samples && e.bad==1);
    ready(&e);
    e.samples=LIMIT-1;
    hold(&e,0,1); assert(!e.fault);
    hold(&e,0,1); assert(e.fault==F_OVERFLOW && e.bad==1);
    ready(&e);
    engine_sample(&e,++tick,16);
    assert(e.fault==F_INPUT && engine_valid(&e));
    /* Startup chatter on the output cannot delay input qualification. */
    engine_init(&e); tick=0;
    for (unsigned i=0;i<20;i++) hold(&e,(i%2)<<2,1);
    assert((e.valid&1) && !(e.valid&2) && e.state==WAIT);
    ready(&e);
    e.now=UINT32_MAX; tick=UINT32_MAX;
    hold(&e,0,1); assert(!e.fault);
    /* Count patterns against an independent integer ratio oracle, both signs. */
    for (unsigned reverse=0; reverse<2; reverse++) {
        for (unsigned divisor=1; divisor<=12; divisor++) {
            ready(&e);
            for (unsigned i=1; i<=192; i++) {
                unsigned input=ring[i%4];
                unsigned j=(i/divisor)%4;
                unsigned output=ring[reverse ? (4-j)%4 : j];
                hold(&e,input|(output<<2),QUALIFY);
            }
            assert(e.good==2 && e.total==192/divisor);
            assert(e.last==192/divisor-96/divisor);
            assert((reverse ? e.opposite : e.same)==2);
        }
    }
    /* A raw glitch on the last rearm interval must cancel that interval. */
    ready(&e); hold(&e,3,QUALIFY);
    assert(e.state==WAIT);
    hold(&e,3,REARM); assert(e.quiet==REARM-1);
    hold(&e,2,1); assert(e.quiet==0);
    hold(&e,3,REARM); assert(e.state==WAIT);
    hold(&e,3,1); assert(e.state==ACTIVE);
    /* Reject deadline equality, but permit the immediately preceding cut. */
    for (unsigned earlier=0; earlier<2; earlier++) {
        ready(&e);
        for (unsigned i=1;i<=95;i++) hold(&e,ring[i%4],QUALIFY);
        e.age=DEADLINE-QUALIFY-earlier;
        hold(&e,0,QUALIFY);
        assert(earlier ? e.good==1 : (e.bad==1 && e.reason==R_TIMEOUT));
    }
    ready(&e);
    for (unsigned i=1;i<=OUTPUT_LIMIT+1;i++)
        hold(&e,ring[i%4]<<2,QUALIFY);
    assert(e.bad==1 && e.reason==R_CAPACITY);
    ready(&e);
    char frame[768];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(!strncmp(frame,"RW1,",4));
    assert(!wire_encode(&e,frame,10));
    ready(&e);
    for (unsigned i=1; i<=96; i++)
        hold(&e,ring[i%4]|(ring[i%4]<<2),QUALIFY);
    e.same=0; e.opposite=1;
    assert(!engine_valid(&e));
    e.same=1; e.opposite=0;
    e.starts++; assert(!engine_valid(&e));
    assert(!wire_encode(&e,frame,sizeof frame));
    puts("PASS quadrature oracle, windows, reversal, jump, timeout, gap, wrap, bounds, CRC");
}
