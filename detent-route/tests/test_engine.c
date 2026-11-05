/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    while (ticks--) engine_sample(e, e->now + 1u, (uint8_t)mask);
}
static void settle(Engine *e, unsigned mask) { hold(e,mask,6); }
static void boot(Engine *e, unsigned station)
{
    engine_init(e); settle(e,1u<<station);
    assert(e->station==station && !e->active);
}
static void move(Engine *e, unsigned station)
{
    settle(e,0); assert(e->active);
    settle(e,1u<<station); assert(!e->active);
    assert(engine_valid(e));
}
static void words(void)
{
    /* Independent ordinal oracle over every six-destination word. */
    for (unsigned code=0;code<4096;code++) {
        Engine e; boot(&e,0);
        unsigned word=code,previous=0,cw=0,ccw=0,ret=0,jump=0,rev=0;
        unsigned direction=0,run=0,lp=0,lm=0;
        for (unsigned i=0;i<6;i++) {
            unsigned next=word%4; word/=4;
            unsigned dir=0;
            if(next==previous) ret++;
            else if(next==(previous+1)%4) {cw++;dir=1;}
            else if(previous==(next+1)%4) {ccw++;dir=2;}
            else jump++;
            if(dir) {
                if(direction && dir!=direction) rev++;
                run=dir==direction?run+1:1;
                if(run==4) {if(dir==1) lp++; else lm++;run=0;}
            } else run=0;
            direction=dir; previous=next;
            move(&e,next);
            assert(e.cw==cw && e.ccw==ccw && e.returns==ret && e.jumps==jump);
            assert(e.reversals==rev && e.run==run && e.direction==dir);
            assert(e.cw_laps==lp && e.ccw_laps==lm);
        }
    }
}
int main(void)
{
    Engine e; boot(&e,0);
    hold(&e,0,5); assert(!e.active);
    hold(&e,0,1); assert(e.active && e.age==0);
    hold(&e,2,5); assert(e.active);
    hold(&e,2,1); assert(!e.active && e.cw==1 && e.last_ms==6);
    move(&e,0); assert(e.ccw==1 && e.reversals==1);
    settle(&e,4); assert(e.shortcuts==1 && e.station==2);
    settle(&e,0); settle(&e,3);
    assert(e.aborted==1 && e.overlaps==1 && e.station==4);
    hold(&e,3,20); assert(e.overlaps==1);
    settle(&e,8); assert(e.station==3);
    settle(&e,0); engine_sample(&e,e.now+2,0);
    assert(e.aborted==2 && e.gaps==1 && !e.active && e.station==4);
    settle(&e,1); settle(&e,0);
    hold(&e,0,DEADLINE_MS-6); settle(&e,2);
    assert(e.timeouts==1 && !e.active && e.cw==1);
    boot(&e,0); settle(&e,0);
    hold(&e,0,DEADLINE_MS-7); settle(&e,2);
    assert(e.cw==1 && e.last_ms==DEADLINE_MS-1);
    boot(&e,0); e.now=UINT32_MAX-3; e.have_sample=true;
    settle(&e,0); settle(&e,8); assert(e.ccw==1 && !e.gaps);
    boot(&e,0); e.starts=UINT32_MAX; e.aborted=UINT32_MAX;
    settle(&e,0); assert(e.fault==F_EXHAUST && !e.active && engine_valid(&e));
    boot(&e,0); settle(&e,0); engine_fault(&e,F_CONFIG);
    assert(e.aborted==1 && !e.active && engine_valid(&e));
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(!wire_encode(&e,frame,8));
    assert(wire_crc("123456789",9)==0x29b1);
    words();
    puts("PASS 4096 route words, qualification, deadline, overlap, shortcut, gap, wrap, exhaustion, CRC");
}
