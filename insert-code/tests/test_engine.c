/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static uint32_t tick;
static void hold(Engine *e, unsigned word, unsigned count)
{
    while (count--) {
        engine_sample(e, ++tick, word);
        assert(engine_valid(e));
    }
}
static void begin(Engine *e)
{
    engine_init(e); tick=0; hold(e,0,6);
    assert(e->state==READY);
}
int main(void)
{
    const unsigned codes[]={3,5,6,9,10,12};
    Engine e;
    /* Every starting code and every later word: independent popcount oracle. */
    for (unsigned i=0;i<6;i++) for(unsigned word=0;word<16;word++) {
        begin(&e); hold(&e,codes[i],6);
        assert(e.state==ACTIVE && e.identity==i+1 && e.seats[i]==1);
        hold(&e,word,6);
        unsigned bits=0; for(unsigned b=0;b<4;b++) bits+=(word>>b)&1u;
        if (!word) assert(e.completed==1 && e.state==READY);
        else if(word==codes[i]) assert(!e.changed && !e.invalid);
        else if(bits==2) assert(e.changed==1 && !e.invalid);
        else assert(e.invalid==1 && !e.changed);
        hold(&e,0,6);
        assert(e.starts==1 && e.completed==1 && !e.abandoned);
    }
    /* Startup attached cannot claim a newly seated insert. */
    engine_init(&e); tick=0; hold(&e,3,30); assert(e.starts==0 && e.state==WAIT_EMPTY);
    hold(&e,0,6); hold(&e,5,5); assert(!e.starts);
    hold(&e,5,1); assert(e.starts==1);
    /* Initial malformed word remains unidentified even if it later recovers. */
    begin(&e); hold(&e,1,6); hold(&e,3,6);
    assert(e.identity==0 && e.unidentified==1 && e.invalid==1 && !e.changed);
    hold(&e,0,6); assert(e.completed==1);
    /* Short empty glitches never authorize a new identity. */
    begin(&e); hold(&e,3,6); hold(&e,0,5); hold(&e,12,6);
    assert(e.starts==1 && e.identity==1 && e.changed==1);
    hold(&e,3,6); hold(&e,12,6); assert(e.changed==1);
    /* Acquisition loss abandons once and requires a new empty baseline. */
    engine_sample(&e,tick+=5,12); assert(e.abandoned==1 && e.state==WAIT_EMPTY);
    hold(&e,12,20); assert(e.starts==1);
    hold(&e,0,6); hold(&e,12,6); assert(e.starts==2);
    engine_fault(&e,F_CONFIG); assert(e.abandoned==2 && e.state==HALT);
    Engine saved=e; hold(&e,0,20); assert(e.samples==saved.samples);
    /* Exhaustion rolls back a potential admission instead of half-credit. */
    begin(&e); e.starts=UINT32_MAX; e.completed=UINT32_MAX;
    e.seats[0]=UINT32_MAX; hold(&e,3,6);
    assert(e.fault==F_OVERFLOW && e.starts==UINT32_MAX && e.seats[0]==UINT32_MAX);
    /* Modular clock wrap is permitted; duplicate time is not. */
    engine_init(&e); engine_sample(&e,UINT32_MAX,0); engine_sample(&e,0,0);
    assert(!e.fault); engine_sample(&e,0,0); assert(e.fault==F_TIME);
    char buffer[1024]; begin(&e); hold(&e,3,6);
    assert(wire_encode(&e,buffer,sizeof buffer)>0);
    assert(wire_encode(&e,buffer,5)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    Engine impossible=e; impossible.qualified=5;
    assert(!engine_valid(&impossible));
    impossible=e; impossible.qualified=1;
    assert(!engine_valid(&impossible));
    e.completed++; assert(!engine_valid(&e));
    puts("PASS: all code/word pairs, qualification, startup, gaps, overflow, wire");
}
