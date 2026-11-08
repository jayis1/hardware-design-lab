/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    while (ticks--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
    }
}
static Engine baseline(unsigned mask)
{
    Engine e; engine_init(&e); hold(&e,mask,30);
    assert(e.state==READY); return e;
}
int main(void)
{
    Engine e=baseline(5);
    hold(&e,0,10); hold(&e,10,30);
    assert(e.completed==1 && e.returned==0 && e.last.origin==5);
    assert(e.last.bins[0]>0 && e.last.bins[4]>0);
    assert(e.last.outcome==TRANSFER && e.last.age>20);
    e=baseline(10); hold(&e,15,10); hold(&e,5,30);
    assert(e.completed==1 && e.last.bins[3]>0 && e.last.bins[7]>0);
    e=baseline(5); hold(&e,9,15); hold(&e,10,30);
    assert(e.completed==1 && e.last.mixed>0);
    e=baseline(5); hold(&e,0,10); hold(&e,5,30);
    assert(e.returned==1 && e.last.outcome==RETURN);
    e=baseline(5); hold(&e,0,10);
    engine_sample(&e,e.now+2,0);
    assert(e.state==HALT && e.aborted==1 && engine_valid(&e));
    e=baseline(5); hold(&e,0,5010);
    assert(e.timeouts==1 && e.last.age==DEADLINE);
    e=baseline(5); hold(&e,0,10);
    e.work.age=DEADLINE-1;
    for(unsigned p=0;p<2;p++) {
        memset(e.work.bins+4*p,0,4*sizeof(uint32_t));
        e.work.unknown[p]=DEADLINE-1;
    }
    e.settle=SETTLE-1; e.target=10; e.raw=e.qualified=10;
    for(unsigned i=0;i<4;i++) {e.candidate[i]=(10>>i)&1; e.stable[i]=QUALIFY;}
    hold(&e,10,1);
    assert(e.timeouts==1 && e.completed==0);
    e=baseline(5); hold(&e,0,10); hold(&e,10,5);
    hold(&e,10,SETTLE-1); assert(!e.completed);
    hold(&e,9,1); assert(!e.completed && !e.settle);
    e=baseline(5); e.now=0xfffffffeu; hold(&e,5,4);
    assert(e.state==READY);
    e=baseline(5); e.starts=LIMIT; e.completed=LIMIT;
    e.last=(Capture){.origin=5,.outcome=TRANSFER,.age=20};
    e.last.bins[2]=20; e.last.bins[6]=20;
    hold(&e,0,5); assert(e.state==HALT && e.fault==F_COUNTER);
    assert(wire_crc("123456789",9)==0x29b1);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame));
    assert(!wire_encode(&e,frame,5));
    /* Exhaust all raw vectors after either baseline, no invented topology. */
    for(unsigned start=5;start<=10;start+=5)
        for(unsigned a=0;a<16;a++) for(unsigned b=0;b<16;b++) {
            e=baseline(start); hold(&e,a,10); hold(&e,b,10); hold(&e,15-start,30);
            for(unsigned p=0;p<2;p++) {
                unsigned sum=e.last.unknown[p];
                for(unsigned k=0;k<4;k++) sum+=e.last.bins[p*4+k];
                assert(sum==e.last.age);
            }
        }
    puts("PASS topology, returns, gap, timeout, deadline tie, raw glitch, wrap, overflow and 512 mask paths");
}
