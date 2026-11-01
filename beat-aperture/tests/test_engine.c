/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void tick(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e,e->now+1,(uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void pulse(Engine *e, unsigned raw)
{
    tick(e,raw,5);
    tick(e,0,5);
}
static Engine fresh(void)
{
    Engine e;
    engine_init(&e);
    tick(&e,0,5);
    assert(e.state==RUN);
    return e;
}
static void oracle_words(void)
{
    /* Independent symbol-list oracle: two alternating endpoint machines and
     * a list of completed B timestamps partitioned at A return endpoints. */
    for (unsigned word=0;word<16384;++word) {
        Engine e=fresh();
        unsigned code=word, phase[2]={0,0}, cycles[2]={0,0};
        unsigned pending=0, bins[3]={0,0,0};
        for (unsigned i=0;i<7;++i) {
            unsigned gate=code%4; code/=4;
            unsigned side=gate/2, right=gate%2;
            if (!right) {
                if (phase[side]==2) {
                    ++cycles[side];
                    if (side && phase[0]) ++pending;
                    if (!side) {
                        ++bins[pending>1?2:pending]; pending=0;
                    }
                }
                phase[side]=1;
            } else if (phase[side]) phase[side]=2;
            pulse(&e,1u<<gate);
            assert(e.side[0].cycles==cycles[0]);
            assert(e.side[1].cycles==cycles[1]);
            assert(e.pending==pending);
            assert(e.zero==bins[0] && e.one==bins[1] && e.many==bins[2]);
        }
    }
}
int main(void)
{
    Engine e=fresh();
    pulse(&e,5); /* simultaneous first left anchors, not completed B */
    pulse(&e,10);
    pulse(&e,5); /* tie belongs to closing aperture */
    assert(e.windows==1 && e.one==1 && e.last_hits==1);
    assert(e.last_first==e.period && e.last_last==e.period);
    assert(e.pending==0 && e.side[0].cycles==1 && e.side[1].cycles==1);
    char frame[768];
    assert(wire_encode(&e,frame,sizeof frame));
    assert(!wire_encode(&e,frame,8));
    assert(wire_crc("123456789",9)==0x29b1);
    e=fresh(); pulse(&e,1); pulse(&e,2); pulse(&e,1);
    assert(e.zero==1 && e.last_hits==0 && !e.last_first);
    e=fresh(); pulse(&e,3); assert(e.state==HALT && (e.faults&F_OVERLAP));
    e=fresh(); pulse(&e,1); engine_sample(&e,e.now+2,0);
    assert(e.state==HALT && e.gaps==1 && !e.windows);
    e=fresh(); pulse(&e,1); tick(&e,0,MAX_CYCLE_MS);
    assert(e.state==HALT && (e.faults&F_TIMEOUT));
    e=fresh(); tick(&e,1,4); tick(&e,0,5); assert(e.side[0].phase==0);
    /* Chatter on B cannot postpone A qualification. */
    e=fresh(); for(unsigned i=0;i<5;++i) tick(&e,1|(i%2?4:0),1);
    assert(e.side[0].phase==1);
    /* Wrap is legal; duplicate time does not progress acquisition. */
    e=fresh(); e.stamp=e.now=UINT32_MAX-2;
    tick(&e,0,5); assert(e.state==RUN && e.now==2);
    Engine before=e; engine_sample(&e,e.now,15); assert(e.levels==before.levels);
    /* Transactional overflow does not leave half of a tied completion. */
    e=fresh(); pulse(&e,5); pulse(&e,10);
    e.side[1].cycles=UINT32_MAX;
    pulse(&e,5); assert(e.state==HALT && e.windows==0 && (e.faults&F_OVERFLOW));
    /* Fastest qualified alternating endpoints: two five-sample plateaus. */
    e=fresh(); tick(&e,1,5); tick(&e,2,5); tick(&e,1,5);
    assert(e.windows==1 && e.period==MIN_CYCLE_MS);
    /* A return at the rejecting deadline must not become a window. */
    e=fresh(); pulse(&e,1); pulse(&e,2);
    tick(&e,0,MAX_CYCLE_MS-e.side[0].age-5);
    tick(&e,1,5); assert(e.state==HALT && !e.windows && (e.faults&F_TIMEOUT));
    /* One sample before the deadline remains admissible. */
    e=fresh(); pulse(&e,1); pulse(&e,2);
    tick(&e,0,MAX_CYCLE_MS-e.side[0].age-6);
    tick(&e,1,5); assert(e.state==RUN && e.windows==1 && e.period==MAX_CYCLE_MS-1);
    /* B completed on the first A anchor does not belong to a prior window. */
    e=fresh(); pulse(&e,4); pulse(&e,8); pulse(&e,5);
    assert(e.side[1].cycles==1 && !e.assigned && !e.pending);
    oracle_words();
    puts("PASS: 16384 endpoint words, ties, zero windows, faults, qualification, wrap and overflow");
    return 0;
}
