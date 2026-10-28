/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned duration)
{
    for (unsigned i=0; i<duration; ++i)
        engine_sample(e, now++, mask);
}
static void ready(Engine *e)
{
    now=0;
    engine_init(e);
    hold(e,15,120);
    assert(e->state==READY);
}
int main(void)
{
    Engine e;
    ready(&e);
    hold(&e,14,20); /* upstream left */
    hold(&e,12,30); /* upstream right */
    hold(&e,8,20);  /* downstream left */
    hold(&e,0,20);  /* downstream right */
    assert(e.result.reason==COMPLETE && e.result.seen==15);
    assert(e.result.at[0]==0 && e.result.at[1]==20);
    assert(e.result.at[2]==50 && e.result.at[3]==70);
    assert(e.result.duration==70 && e.result.id==1);
    char frame[256];
    assert(wire_encode(&e,frame,sizeof frame)>0);
    FILE *f=fopen("build/fixture.txt","w"); assert(f);
    fputs(frame,f); fclose(f);
    hold(&e,0,100); assert(e.result.id==1);
    hold(&e,15,120); assert(e.state==READY);
    hold(&e,11,10); assert(e.result.reason==ORDER);
    ready(&e); hold(&e,14,20); hold(&e,15,10);
    assert(e.result.reason==RECLOSE);
    ready(&e); hold(&e,14,20); now+=10;
    engine_sample(&e,now++,14);
    assert(e.result.reason==GAP && e.gaps==1);
    hold(&e,15,120); assert(e.state==READY);
    ready(&e); hold(&e,14,MAX_TRIAL_MS+10);
    assert(e.result.reason==TIMEOUT);
    ready(&e); hold(&e,0,10);
    assert(e.result.reason==COMPLETE && e.result.duration==0);
    ready(&e); hold(&e,14,3); hold(&e,15,10);
    assert(e.state==READY && e.result.id==0);
    /* Independent debounce: right chatter must not hide left departure. */
    hold(&e,14,2); hold(&e,12,2); hold(&e,14,3);
    assert(e.state==ACTIVE && e.seen==1);
    ready(&e); now=UINT32_MAX-50; e.last=now-1;
    hold(&e,14,20); hold(&e,0,50);
    assert(e.result.reason==COMPLETE && e.result.duration==20);
    ready(&e); hold(&e,0,10); hold(&e,15,120);
    e.result.id=UINT32_MAX;
    hold(&e,0,10); assert(e.state==FAULT);
    assert(wire_encode(&e,frame,sizeof frame)>0);
    ready(&e); assert(wire_encode(&e,frame,4)==0);
    e.result.reason=COMPLETE; assert(wire_encode(&e,frame,sizeof frame)==0);
    /* Deadline precedence: all-open qualifies on, not before, timeout. */
    ready(&e); hold(&e,14,10);
    while ((uint32_t)(now-e.start) < MAX_TRIAL_MS-QUALIFY_MS)
        hold(&e,14,1);
    hold(&e,0,QUALIFY_MS+1);
    assert(e.result.reason==TIMEOUT);
    ready(&e); e.tx_errors=UINT32_MAX; engine_tx_error(&e);
    assert(e.state==FAULT && e.faults==F_OVERFLOW);
    assert(wire_encode(&e,frame,sizeof frame)>0);
    /* All channel permutations: six upstream-respecting orders succeed. */
    unsigned accepted=0;
    for (unsigned a=0;a<4;++a)
    for (unsigned b=0;b<4;++b)
    for (unsigned c=0;c<4;++c)
    for (unsigned d=0;d<4;++d) {
        if (a==b || a==c || a==d || b==c || b==d || c==d) continue;
        unsigned order[4]={a,b,c,d};
        unsigned mask=15;
        ready(&e);
        for (unsigned i=0;i<4;++i) {
            mask &= ~(1u<<order[i]);
            hold(&e,mask,10);
            assert(engine_valid(&e));
        }
        if (e.result.reason==COMPLETE) ++accepted;
        else assert(e.result.reason==ORDER);
    }
    assert(accepted==6);
    puts("PASS peel ordering, skew offsets, debounce, gap, wrap, timeout, exhaustion, encoder");
}
