/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void step(Engine *e, unsigned raw)
{
    engine_sample(e, e->now + 1u, raw);
    assert(engine_valid(e));
}
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) step(e, raw);
}
static unsigned majority(unsigned raw)
{
    return ((raw&1u)!=0) + ((raw&2u)!=0) + ((raw&4u)!=0) >= 2;
}
int main(void)
{
    Engine e;
    /* Independent truth table for every stationary four-contact word. */
    for (unsigned raw=0; raw<16; ++raw) {
        engine_init(&e);
        hold(&e,raw,6);
        assert(e.valid==15 && e.voter_known==0 && e.compare_known==0);
        hold(&e,raw,20);
        assert(e.voter_known==20 && e.compare_known==20);
        for (unsigned i=0;i<5;++i) {
            unsigned on = i<3 ? (((raw>>i)&1u)!=majority(raw)) :
                (i==3 ? majority(raw) && !(raw&8u) : !majority(raw) && (raw&8u));
            assert(e.total[i]==20*on && e.peak[i]==20*on);
            assert(e.segments[i]==on && e.run[i]==20*on);
        }
    }
    /* All ordered stable pairs, preserving old evidence without bin migration. */
    for (unsigned a=0;a<16;++a) for (unsigned b=0;b<16;++b) {
        engine_init(&e); hold(&e,a,20);
        Engine before=e;
        hold(&e,b,20);
        for (unsigned i=0;i<3;++i) {
            unsigned on=(((b>>i)&1u)!=majority(b));
            unsigned credit=(a&7u)==(b&7u) ? 20 : 14;
            assert(e.total[i]-before.total[i]==on*credit);
        }
        assert(e.compare_known-before.compare_known==(a==b?20:14));
    }
    /* Chattering reference must not hide stable minority evidence. */
    engine_init(&e); hold(&e,1,10);
    unsigned known=e.voter_known, total=e.total[0];
    for(unsigned i=0;i<100;++i) step(&e,1u|((i&1u)<<3));
    assert(e.voter_known==known+100 && e.total[0]==total+100);
    assert(e.compare_known<e.voter_known);
    /* A rejected voter glitch splits an observed segment and resets its run. */
    engine_init(&e); hold(&e,1,20);
    unsigned segments=e.segments[0];
    step(&e,0); assert(e.run[0]==0);
    hold(&e,1,7); assert(e.segments[0]==segments+1);
    /* Missing ticks retain history, invalidate qualification, and split runs. */
    total=e.total[0];
    engine_sample(&e,e.now+10,1);
    assert(e.gaps==1 && e.missed==9 && e.valid==0 && e.total[0]==total);
    hold(&e,1,5); assert(e.total[0]==total);
    step(&e,1); assert(e.total[0]==total+1);
    assert(engine_valid(&e));
    /* Modular wrap is legal; repeated/reversed time is terminal. */
    engine_init(&e); engine_sample(&e,UINT32_MAX-2,1);
    hold(&e,1,20); assert(!e.fault);
    engine_sample(&e,e.now,1); assert(e.fault==F_TIME && e.valid==0);
    Engine frozen=e; hold(&e,15,20);
    assert(e.samples==frozen.samples && e.total[0]==frozen.total[0]);
    engine_init(&e); step(&e,16); assert(e.fault==F_INPUT);
    /* Staged increments: a failed sample cannot partly advance counters. */
    engine_init(&e); hold(&e,1,10); e.tx_errors=UINT32_MAX;
    engine_tx_error(&e); assert(e.fault==F_OVERFLOW && engine_valid(&e));
    engine_init(&e); hold(&e,1,10);
    e.samples=UINT32_MAX; e.intervals=UINT32_MAX-1;
    assert(engine_valid(&e));
    frozen=e; step(&e,1);
    assert(e.fault==F_OVERFLOW && e.intervals==frozen.intervals);
    assert(e.total[0]==frozen.total[0]);
    /* CRC reference vector and bounded serialization. */
    assert(wire_crc("123456789",9)==0x29b1);
    char frame[1024]; size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n>0 && frame[n-1]=='\n' && !memcmp(frame,"QC1,",4));
    assert(!wire_encode(&e,frame,5));
    e.total[0]=0; assert(!engine_valid(&e));
    assert(!wire_encode(&e,frame,sizeof frame));
    puts("PASS truth table, 256 transitions, independent qualification, glitches, gaps, wrap, faults, overflow and wire bounds");
}
