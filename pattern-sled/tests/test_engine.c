/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint32_t clock_ms;
static void run(Engine *e, unsigned raw, unsigned n)
{
    while (n--) engine_sample(e, ++clock_ms, (uint8_t)raw);
    assert(engine_valid(e));
}
static void init(Engine *e)
{
    clock_ms=0;
    engine_init(e);
    run(e,0,21);
}
static void word(Engine *e,unsigned data)
{
    run(e,data<<1,25);
    run(e,(data<<1)|1,12);
    run(e,data<<1,1);
}
static void normal(void)
{
    Engine e; init(&e);
    for(unsigned i=0;i<16;i++) word(&e,i%8);
    assert(e.words==16 && e.bad==0);
    for(unsigned i=0;i<16;i++) word(&e,i%8);
    assert(e.words==32 && e.bad==0);
    for(unsigned i=0;i<16;i++) word(&e,(i%8)^1);
    assert(e.words==48 && e.bad==16);
    for(unsigned i=0;i<16;i++) {
        assert(e.reference[i]==i%8);
        assert(e.errors[i]==1);
        assert(e.latest[i]==((i%8)^1));
    }
    char frame[768]; size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n>0 && n<512 && frame[n-1]=='\n');
    assert(wire_encode(&e,frame,1)==0);
    if (getenv("PATTERN_FIXTURE")) fwrite(frame,1,n,stdout);
}
static void faults(void)
{
    Engine e; init(&e);
    run(&e,3,1); assert(e.fault==F_SETUP && e.words==0);
    init(&e); run(&e,1,5); run(&e,0,1);
    assert(e.fault==F_SHORT && e.words==0);
    init(&e); run(&e,1,12); run(&e,2,1);
    assert(e.fault==F_HOLD && e.words==0);
    init(&e); run(&e,1,1001);
    assert(e.fault==F_LONG && e.words==0);
    init(&e); word(&e,0); run(&e,1,1);
    assert(e.fault==F_REARM && e.words==1);
    init(&e); engine_sample(&e,clock_ms+2,0);
    assert(e.fault==F_GAP && e.words==0);
    Engine before=e; run(&e,15,100);
    assert(e.words==before.words && e.bad==before.bad);
    assert(!memcmp(e.reference,before.reference,sizeof e.reference));
}
static void boundaries(void)
{
    Engine e; init(&e); run(&e,1,10); run(&e,0,1);
    assert(e.words==1 && !e.fault);
    init(&e); run(&e,1,999); run(&e,0,1);
    assert(e.words==1 && !e.fault);
    init(&e); run(&e,1,1000); run(&e,0,1);
    assert(e.words==0 && e.fault==F_LONG);
    engine_init(&e); clock_ms=0; run(&e,1,30); run(&e,0,21);
    word(&e,5); assert(e.words==1 && e.reference[0]==5);
    engine_init(&e); clock_ms=UINT32_MAX-10; run(&e,0,21);
    word(&e,3); assert(e.words==1 && !e.fault);
}
static void oracle(void)
{
    Engine e; init(&e); uint32_t seed=9, bad=0, errors[16]={0};
    unsigned ref[16]={0};
    for(unsigned n=0;n<10000;n++) {
        seed=seed*1664525u+1013904223u;
        unsigned value=(seed>>24)&7, slot=n%16;
        if(n<16) ref[slot]=value;
        else if(value!=ref[slot]) { ++bad; ++errors[slot]; }
        word(&e,value);
        assert(e.words==n+1 && e.bad==bad);
        for(unsigned j=0;j<16;j++) assert(e.errors[j]==errors[j]);
    }
}
static void exhaustion_and_invariants(void)
{
    Engine e; init(&e);
    for(unsigned delay=19;delay<=21;delay++) {
        init(&e); word(&e,0);
        run(&e,0,delay-1); run(&e,1,1);
        assert(e.fault==(delay<20 ? F_REARM:F_NONE));
    }
    init(&e);
    e.words=UINT32_MAX;
    word(&e,0);
    assert(e.fault==F_OVERFLOW && e.words==UINT32_MAX && e.bad==0);
    for(unsigned i=0;i<16;i++) assert(e.errors[i]==0 && e.latest[i]==0);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame)>0);
    init(&e); e.tx=UINT32_MAX; engine_tx_error(&e);
    assert(e.fault==F_OVERFLOW && engine_valid(&e));
    init(&e); e.bad=1; assert(!engine_valid(&e));
    assert(!wire_encode(&e,frame,sizeof frame));
    init(&e); e.words=17; e.latest[0]=1;
    assert(!engine_valid(&e));
    e.errors[0]=1; e.bad=1; assert(engine_valid(&e));
    e.latest[0]=0; assert(!engine_valid(&e));
    /* Setup boundary: nineteen preceding stable intervals plus the rising
     * endpoint make twenty. Eighteen plus that endpoint remains too short. */
    init(&e); run(&e,2,19); run(&e,3,1);
    assert(e.fault==F_SETUP);
    init(&e); run(&e,2,20); run(&e,3,10); run(&e,2,1);
    assert(e.words==1 && !e.fault);
    for(unsigned before=0;before<8;before++) {
        for(unsigned after=0;after<8;after++) {
            init(&e); run(&e,before<<1,25); run(&e,(before<<1)|1,12);
            run(&e,after<<1,1);
            assert(e.words==(before==after));
            assert(e.fault==(before==after ? F_NONE:F_HOLD));
        }
    }
}
int main(void)
{
    normal(); faults(); boundaries(); oracle(); exhaustion_and_invariants();
    puts("PASS pattern engine: learning, 10000-word oracle, timing, faults, wire");
    return 0;
}
