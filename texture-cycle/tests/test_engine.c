/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t tick;
static void hold(Engine *e,unsigned raw,unsigned n)
{
    while(n--) { engine_sample(e,++tick,(uint8_t)raw); assert(engine_valid(e)); }
}
static unsigned gray(unsigned n) { return n^(n>>1); }
static void press(Engine *e,unsigned id,unsigned key)
{
    hold(e,gray(id),8); hold(e,gray(id)|key,8); hold(e,gray(id),8);
}
static void edge(Engine *e,unsigned winner,unsigned loser)
{
    press(e,loser,4); press(e,winner,8);
}
static Engine fresh(void)
{
    Engine e; engine_init(&e); tick=0; hold(&e,0,8); return e;
}
static int oracle_cycle(unsigned graph)
{
    /* Independent bounded walk search; any returning walk has a cycle. */
    for(unsigned origin=0;origin<4;origin++) {
        unsigned reachable=1u<<origin;
        for(unsigned depth=0;depth<4;depth++) {
            unsigned next=0;
            for(unsigned a=0;a<4;a++) if(reachable&(1u<<a))
                for(unsigned b=0;b<4;b++) if(graph&(1u<<(4*a+b))) next|=1u<<b;
            if(next&(1u<<origin)) return 1;
            reachable=next;
        }
    }
    return 0;
}
int main(void)
{
    Engine e=fresh();
    press(&e,1,8); assert(e.rejected==1 && !e.graph);
    press(&e,1,4); press(&e,1,8); assert(e.rejected==2 && !e.anchored);
    edge(&e,0,1); edge(&e,1,2); assert(!e.cycle);
    edge(&e,2,0); assert(e.cycle && e.preferences==3);
    edge(&e,0,1); assert(e.preferences==4 && e.graph==((1u<<1)|(1u<<6)|(1u<<8)));
    press(&e,0,8); assert(e.rejected==3); /* reference consumed */
    e=fresh(); press(&e,2,4);
    hold(&e,gray(1),8); hold(&e,gray(1)|8,6);
    hold(&e,gray(0)|8,1); hold(&e,gray(1)|8,3); hold(&e,gray(1),8);
    assert(e.rejected==1 && !e.graph && !e.anchored);
    e=fresh(); press(&e,2,4); hold(&e,0,8); hold(&e,12,8); hold(&e,0,8);
    assert(e.rejected==1 && !e.anchored);
    e=fresh(); press(&e,2,4); hold(&e,0,8);
    hold(&e,4,1); hold(&e,8,5); hold(&e,0,8);
    assert(e.rejected==1 && !e.preferences && !e.anchored);
    e=fresh(); press(&e,2,4); hold(&e,8,8);
    engine_sample(&e,tick+3,8); tick+=3;
    assert(e.aborted==1 && e.gaps==1 && !e.anchored); hold(&e,0,8);
    e=fresh(); press(&e,2,4); hold(&e,8,5);
    hold(&e,8,MAX_PRESS_MS); assert(e.aborted==1 && !e.anchored);
    /* Deadline-minus-one succeeds; exact deadline aborts on release. */
    for(unsigned exact=0;exact<2;exact++) {
        e=fresh(); press(&e,2,4); hold(&e,0,8); hold(&e,8,5);
        hold(&e,8,MAX_PRESS_MS-QUALIFY-1+exact); hold(&e,0,QUALIFY);
        assert(e.preferences==1-exact && e.aborted==exact);
    }
    e=fresh(); press(&e,1,4); press(&e,3,4); press(&e,0,8);
    assert(e.references==2 && e.preferences==1 && e.graph==(1u<<3));
    press(&e,2,4); engine_sample(&e,tick+2,0); tick+=2;
    assert(!e.anchored && e.graph==(1u<<3));
    e=fresh(); e.now=e.stamp=UINT32_MAX; tick=UINT32_MAX;
    hold(&e,0,1); assert(e.now==0 && !e.gaps);
    Engine bad=e; bad.graph=1; assert(!engine_valid(&bad));
    char invalid[768]; assert(!wire_encode(&bad,invalid,sizeof invalid));
    e=fresh(); e.starts=UINT32_MAX; e.rejected=UINT32_MAX;
    hold(&e,4,8); assert(e.state==HALT && (e.faults&F_OVERFLOW));
    /* All 4096 directed irreflexive graphs, built by real contact presses. */
    for(unsigned mask=0;mask<4096;mask++) {
        e=fresh(); unsigned expected=0,bit=0;
        for(unsigned a=0;a<4;a++) for(unsigned b=0;b<4;b++) if(a!=b) {
            if(mask&(1u<<bit)) { edge(&e,a,b); expected|=1u<<(4*a+b); }
            bit++;
        }
        assert(e.graph==expected); assert(e.cycle==(unsigned)oracle_cycle(expected));
        char out[768]; assert(wire_encode(&e,out,sizeof out)>0);
        assert(!wire_encode(&e,out,4));
    }
    puts("PASS: all 4096 preference graphs, envelope faults, orphan/self, repeat, gap, timeout, exhaustion");
    return 0;
}
