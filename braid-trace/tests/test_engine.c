/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static void ticks(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e,e->now+1,(uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void start(Engine *e)
{
    engine_init(e);
    ticks(e,1,6);
    assert(e->state==READY);
}
static void crossing(Engine *e,unsigned code)
{
    unsigned pair=(code-1)%2+1;
    ticks(e,pair,6);
    ticks(e,pair+(code<=2?4:8),6);
    ticks(e,pair,6);
}
static void oracle(unsigned word,unsigned length)
{
    Engine e; start(&e);
    unsigned stack[32],depth=0,p[3]={0,1,2},pos=0,neg=0;
    for (unsigned i=0;i<length;++i) {
        unsigned c=(word>>(2*i))%4+1;
        crossing(&e,c);
        unsigned slot=(c-1)%2,swap=p[slot];
        p[slot]=p[slot+1]; p[slot+1]=swap;
        if (c<=2) ++pos; else ++neg;
        if (depth && ((stack[depth-1]+1)%4+1)==c) --depth;
        else stack[depth++]=c;
        assert(e.accepted==i+1 && e.depth==depth);
        assert(e.positive==pos && e.negative==neg);
        assert(e.permutation==p[0]+3*p[1]+9*p[2]);
        for (unsigned j=0;j<depth;++j) assert(e.reduced[j]==stack[j]);
    }
}
int main(void)
{
    /* Exhaust every signed adjacent-generator word through length six. */
    for (unsigned n=0;n<=6;++n)
        for (unsigned w=0;w<(1u<<(2*n));++w) oracle(w,n);
    Engine e; start(&e);
    crossing(&e,1); crossing(&e,3);
    assert(e.depth==0 && e.cancelled==1 && e.accepted==2);
    crossing(&e,1); crossing(&e,1);
    assert(e.permutation==21 && e.depth==2); /* permutation identity != empty braid */
    start(&e);
    ticks(&e,5,6); ticks(&e,6,1); ticks(&e,1,6);
    assert(e.rejected==1 && !e.accepted); /* raw detent loss during stroke */
    start(&e);
    ticks(&e,5,1); ticks(&e,9,6); ticks(&e,1,6);
    assert(e.rejected==1); /* raw paddle handoff is not an UNDER stroke */
    start(&e); ticks(&e,13,6); ticks(&e,1,6); assert(e.rejected==1);
    start(&e); ticks(&e,0,6); ticks(&e,4,6); ticks(&e,0,6); assert(e.rejected==1);
    start(&e); ticks(&e,3,6); ticks(&e,7,6); ticks(&e,3,6); assert(e.rejected==1);
    start(&e); ticks(&e,5,5);
    assert(e.state==ACTIVE && e.age==0);
    ticks(&e,5,MAX_PRESS_MS-5); ticks(&e,1,5);
    assert(e.state==HALT && e.aborted==1 && (e.faults&F_TIMEOUT));
    start(&e); crossing(&e,2); ticks(&e,5,6);
    engine_sample(&e,e.now+2,1);
    assert(engine_valid(&e) && e.state==HALT && e.accepted==1 && e.aborted==1);
    unsigned a=e.accepted; ticks(&e,9,20); assert(e.accepted==a);
    start(&e); for (unsigned i=0;i<32;++i) crossing(&e,i%4+1);
    assert(e.accepted==32); crossing(&e,1);
    assert(e.state==HALT && e.accepted==32 && e.aborted==1 && (e.faults&F_CAPACITY));
    start(&e); e.starts=UINT32_MAX; e.rejected=UINT32_MAX;
    ticks(&e,5,6); assert(e.state==HALT && e.starts==UINT32_MAX);
    start(&e); e.now=UINT32_MAX-3; e.stamp=e.now;
    ticks(&e,1,8); assert(e.state==READY && !e.gaps);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame));
    assert(!wire_encode(&e,frame,4));
    e.depth=1; assert(!engine_valid(&e) && !wire_encode(&e,frame,sizeof frame));
    puts("PASS engine: exhaustive words, raw guards, limits, wrap, encoding");
}
