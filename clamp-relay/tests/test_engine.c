/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    for (unsigned i=0; i<ticks; ++i) engine_sample(e, ++now, mask);
}
static Engine fresh(unsigned mask)
{
    Engine e;
    now=0;
    engine_init(&e);
    hold(&e,mask,60);
    assert(e.owner==(mask==3?1:2));
    return e;
}
int main(int argc, char **argv)
{
    Engine e=fresh(3);
    hold(&e,15,30); hold(&e,12,60);
    assert(e.starts==1 && e.completed==1 && e.aborted==0);
    assert(e.result==DONE && e.overlap_ms==30 && e.gap_ms==0);
    assert(e.owner==2 && e.direction==1);
    char frame[512];
    size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n && frame[n-1]=='\n');
    if(argc>1 && !strcmp(argv[1],"fixture")) fputs(frame,stdout);
    assert(!wire_encode(&e,frame,10));
    assert(wire_crc("123456789",9)==0x29b1);
    /* Closed-form duration oracle for symmetric qualified transitions.
     * Equal qualification delays cancel, so held duration is exact here. */
    for(unsigned origin=0; origin<2; ++origin) {
        for(unsigned mode=0; mode<2; ++mode) {
            for(unsigned duration=6; duration<=200; ++duration) {
                unsigned from=origin?12:3, to=origin?3:12;
                e=fresh(from);
                hold(&e,mode?15:0,duration); hold(&e,to,60);
                assert(e.completed==1 && e.elapsed_ms==duration+50);
                assert(e.gap_ms==(mode?0:duration));
                assert(e.overlap_ms==(mode?duration:0));
            }
        }
    }
    e=fresh(3); hold(&e,0,20); hold(&e,12,60);
    assert(e.completed==1 && e.gap_ms==20 && e.max_gap_ms==20);
    e=fresh(12); hold(&e,15,20); hold(&e,3,60);
    assert(e.completed==1 && e.direction==2);
    e=fresh(3); hold(&e,15,20); hold(&e,3,60);
    assert(e.aborted==1 && e.result==RETURNED && e.owner==1);
    e=fresh(3); hold(&e,12,60);
    assert(e.completed==1 && e.direct==1 && !e.gap_ms && !e.overlap_ms);
    e=fresh(3); hold(&e,0,20); now+=10; engine_sample(&e,now,12);
    assert(e.result==GAP && e.aborted==1 && e.gaps==1 && !e.owner);
    hold(&e,12,60); assert(e.owner==2 && e.starts==1);
    e=fresh(3); hold(&e,0,10020);
    assert(e.result==TIMEOUT && e.aborted==1 && e.elapsed_ms==10000);
    hold(&e,12,60); assert(e.owner==2);
    hold(&e,15,20); hold(&e,3,60);
    assert(e.completed==1 && e.aborted==1 && e.starts==2);
    /* Terminal fault during active work accounts the abandoned reservation. */
    e=fresh(3); hold(&e,0,20); engine_fault(&e,F_WATCHDOG);
    assert(e.aborted==1 && e.result==FAULT && engine_valid(&e));
    uint32_t frozen=e.elapsed_ms; hold(&e,12,60);
    assert(e.elapsed_ms==frozen && e.completed==0);
    /* Chatter on B must not postpone qualification of the A jaw. */
    e=fresh(3);
    for(unsigned i=0;i<10;++i) hold(&e,2|((i&1)?4:0),1);
    assert(!(e.levels&1) && e.starts==1);
    /* Raw glitches shorter than qualification do not create transfers. */
    e=fresh(3); hold(&e,0,4); hold(&e,3,10); assert(!e.starts);
    /* Sheet absent is not effective holding even with the jaw closed. */
    e=fresh(3); hold(&e,5,20); assert(e.result==ACTIVE && e.grips==0);
    /* Clock rollover is legal for consecutive sampling. */
    e=fresh(3); e.now=UINT32_MAX-5; now=e.now;
    hold(&e,15,20); hold(&e,12,60); assert(e.completed==1 && !e.gaps);
    /* Fault conservation and serialization survive exhaustion. */
    e=fresh(3); e.starts=e.completed=UINT32_MAX; e.direction=1;
    hold(&e,0,10); assert(e.faults && e.result==FAULT && e.starts==e.completed);
    assert(wire_encode(&e,frame,sizeof frame));
    /* All masks, repeated samples and gaps preserve conservation. */
    engine_init(&e); now=0;
    uint32_t random=1234;
    for(unsigned i=0;i<50000;++i) {
        random=random*1664525u+1013904223u;
        hold(&e,(random>>16)&15,random%12+1);
        if(i%97==0) { now+=10; engine_sample(&e,now,0); }
        assert(engine_valid(&e));
        assert(wire_encode(&e,frame,sizeof frame));
    }
    if(argc==1) puts("PASS transfer ordering, durations, gap, timeout, chatter, wrap, overflow and 50000 randomized transitions");
    return 0;
}
