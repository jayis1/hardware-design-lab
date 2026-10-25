/* Author: jayis1. SPDX-License-Identifier: GPL-2.0-only */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned ms)
{
    for (unsigned i=0;i<ms;++i) engine_sample(e,mask,++now);
}
static void ready(Engine *e)
{
    engine_init(e,now);
    hold(e,UP|LATCH,1100);
    assert(e->state==READY);
}
static void start(Engine *e)
{
    hold(e,UP|LATCH|MARK,30);
    assert(e->state==CAST);
    hold(e,UP|LATCH,30);
}
int main(void)
{
    Engine e;
    now=0;
    ready(&e);
    start(&e);
    hold(&e,UP|LATCH,1100);
    hold(&e,LATCH,50);
    assert(e.state==TURN);
    hold(&e,DOWN|LATCH,40);
    assert(e.state==DRAIN);
    hold(&e,DOWN|LATCH,1100);
    hold(&e,LATCH,40);
    hold(&e,UP|LATCH,40);
    assert(e.state==REST);
    hold(&e,UP,40);
    assert(e.state==DONE && e.completed==1);
    assert(e.residence_ms>=1000 && e.drain_ms>=1000);
    char out[256];
    assert(wire_status(&e,7,out,sizeof out)>0);
    assert(strncmp(out,"$SW1,7,",7)==0);
    assert(wire_status(&e,7,out,8)==0);
    /* No reset of results until a new marked cycle starts. */
    hold(&e,UP|LATCH,1100);
    assert(e.state==READY && e.completed==1 && e.drain_ms>=1000);
    start(&e);
    hold(&e,UP,30);
    assert(e.state==REJECT && e.reason==OPEN_EARLY);
    ready(&e);
    start(&e);
    hold(&e,UP|DOWN|LATCH,30);
    assert(e.state==REJECT && e.reason==CONTRADICTION);
    ready(&e);
    start(&e);
    engine_sample(&e,UP|LATCH,now+=5);
    assert(e.state==REJECT && e.reason==SAMPLE_GAP && e.gaps==1);
    ready(&e);
    start(&e);
    hold(&e,LATCH,30);
    hold(&e,DOWN|LATCH,30);
    hold(&e,LATCH,30);
    hold(&e,DOWN|LATCH,30);
    assert(e.state==DRAIN && e.interruptions==1);
    assert(e.longest_drain_ms<e.drain_ms);
    ready(&e);
    start(&e);
    hold(&e,UP|LATCH|MARK,30);
    assert(e.reason==MARK_AGAIN && e.state==REJECT);
    now=UINT32_MAX-1500;
    ready(&e);
    start(&e);
    hold(&e,UP|LATCH,2000);
    assert(e.state==CAST && e.residence_ms>=2000);
    e.residence_ms=UINT32_MAX;
    hold(&e,UP|LATCH,1);
    assert(e.state==REJECT && e.reason==COUNTER_LIMIT);
    ready(&e);
    start(&e);
    hold(&e,LATCH,TURN_LIMIT_MS+30);
    assert(e.state==REJECT && e.reason==TURN_TIMEOUT);
    ready(&e);
    hold(&e,UP|LATCH|MARK,5);
    hold(&e,UP|LATCH,30);
    assert(e.state==READY);
    /* Startup with a held marker cannot start a cast. */
    engine_init(&e,now);
    hold(&e,UP|LATCH|MARK,2000);
    assert(e.state==CLEAR);
    hold(&e,UP|LATCH,1100);
    assert(e.state==READY);
    engine_transport_loss(&e);
    assert(e.transport_losses==1);
    ready(&e);
    start(&e);
    hold(&e,LATCH,30);
    hold(&e,UP|LATCH,30);
    assert(e.state==REJECT && e.reason==NO_DRAIN);
    ready(&e);
    start(&e);
    e.cycle_ms=CYCLE_LIMIT_MS-1;
    hold(&e,UP|LATCH,1);
    assert(e.state==REJECT && e.reason==CYCLE_TIMEOUT);
    ready(&e);
    start(&e);
    hold(&e,DOWN|LATCH,100);
    hold(&e,UP|LATCH,100);
    assert(e.state==REST && e.drain_ms==100);
    hold(&e,DOWN|LATCH,100);
    hold(&e,UP|LATCH,100);
    assert(e.interruptions==1 && e.drain_ms==200);
    assert(e.longest_drain_ms==100 && e.rest_ms==180);
    e.completed=UINT32_MAX;
    hold(&e,UP,30);
    assert(e.state==REJECT && e.reason==COUNTER_LIMIT);
    /* Whole-mask qualification: the nineteenth sample is still unknown. */
    engine_init(&e,now);
    hold(&e,UP|LATCH,19);
    assert(!e.qualified);
    hold(&e,UP|LATCH,1);
    assert(e.qualified && e.state==CLEAR);
    hold(&e,UP|LATCH,998);
    assert(e.state==CLEAR);
    hold(&e,UP|LATCH,1);
    assert(e.state==READY);
    assert(wire_crc("123456789",9)==0x29b1u);
    puts("PASS: casting, drain interruption, gaps, chatter, wrap, faults, framing");
    return 0;
}
