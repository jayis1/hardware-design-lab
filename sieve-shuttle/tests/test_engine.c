/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, uint8_t raw, unsigned ms)
{
    while (ms--) {
        engine_sample(e, ++tick, raw);
        assert(engine_valid(e));
    }
}
static void baseline(Engine *e)
{
    tick=0;
    engine_init(e);
    hold(e,0,120);
    assert(e->state==ARMED);
}
static void visit(Engine *e, unsigned id)
{
    hold(e,(uint8_t)(1u<<id),20);
    hold(e,0,120);
}
static void words(void)
{
    /* Independent oracle over every six-zone word, including repetitions. */
    for (unsigned word=0; word<4096; ++word) {
        Engine e;
        baseline(&e);
        unsigned matrix[16]={0}, x=0,y=0,returns=0,turns=0;
        unsigned prior=4, run=0, axis=2, peaks[2]={0};
        for(unsigned k=0;k<6;++k) {
            unsigned id=(word>>(2*k))&3;
            visit(&e,id);
            if(prior<4) {
                ++matrix[4*prior+id];
                if(id==prior) { ++returns; run=0; axis=2; }
                else if((id/2)==(prior/2)) {
                    unsigned a=id/2;
                    if(a==0) ++x; else ++y;
                    run=axis==a?run+1:1; axis=a;
                    if(run>peaks[a]) peaks[a]=run;
                } else { ++turns; run=0; axis=2; }
            }
            prior=id;
            assert(e.visits==k+1 && e.last==id);
            assert(e.x==x && e.y==y && e.returns==returns && e.turns==turns);
            assert(e.run==run && e.axis==axis);
            assert(e.peak_x==peaks[0] && e.peak_y==peaks[1]);
            for(unsigned j=0;j<16;++j) assert(e.matrix[j]==matrix[j]);
        }
    }
}
int main(int argc,char **argv)
{
    Engine e;
    char frame[768];
    baseline(&e);
    if(argc>1 && strcmp(argv[1],"--fixture")==0) {
        wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        for(unsigned i=0;i<8;++i) {
            visit(&e,i%4);
            wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        }
        return 0;
    }
    words();
    baseline(&e);
    hold(&e,1,20); hold(&e,0,105);
    assert(e.visits==0); /* first low candidate + qualification + 99 ms */
    hold(&e,0,1); assert(e.visits==1);
    visit(&e,1); assert(e.x==1);
    /* Raw competing activity during clearing cannot invent a unique arrival. */
    hold(&e,1,20); hold(&e,0,50); hold(&e,4,1);
    assert(e.state==HALT && (e.fault & F_AMBIGUOUS));
    unsigned retained=e.visits;
    hold(&e,0,200); assert(e.visits==retained);
    baseline(&e); hold(&e,3,1); assert(e.state==HALT);
    baseline(&e); hold(&e,1,3); hold(&e,0,150); assert(e.visits==0);
    baseline(&e); hold(&e,1,20);
    engine_sample(&e,tick+=6,1); assert(e.state==HALT && e.gaps==1);
    baseline(&e); hold(&e,1,30020); assert(e.state==HALT && (e.fault&F_TIMEOUT));
    baseline(&e); visit(&e,0);
    e.visits=COUNT_LIMIT; e.returns=COUNT_LIMIT-1;
    e.matrix[0]=COUNT_LIMIT-1;
    assert(engine_valid(&e));
    visit(&e,0); assert(e.state==HALT && e.visits==COUNT_LIMIT);
    assert(e.returns==COUNT_LIMIT-1 && (e.fault&F_COUNTER));
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,5)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    baseline(&e); e.matrix[0]=1; assert(!engine_valid(&e));
    baseline(&e); e.qualified=0; assert(!engine_valid(&e));
    baseline(&e); tick=0xfffffff0u; e.now=tick;
    hold(&e,0,40); visit(&e,2); visit(&e,3); assert(e.y==1);
    puts("PASS: 4096 six-zone words, boundaries, faults, overflow, CRC, wrap");
    return 0;
}
