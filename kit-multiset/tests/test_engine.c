/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t t;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) engine_sample(e, ++t, (uint8_t)raw);
}
static void setup(Engine *e)
{
    t = 0; engine_init(e); hold(e, 0, 10);
    assert(e->state == READY);
    hold(e, 8, 10); assert(e->state == ACTIVE);
}
static void stroke(Engine *e, unsigned bin)
{
    hold(e, 8u | (1u << bin), 10); hold(e, 8, 10);
}
int main(void)
{
    Engine e;
    /* All twelve distinct permutations of A,A,B,C are legal. */
    unsigned tested = 0;
    for (unsigned a=0; a<3; ++a) for (unsigned b=0; b<3; ++b)
    for (unsigned c=0; c<3; ++c) for (unsigned d=0; d<3; ++d) {
        unsigned count[3]={0}; ++count[a]; ++count[b]; ++count[c]; ++count[d];
        if (count[0]!=2 || count[1]!=1 || count[2]!=1) continue;
        setup(&e); stroke(&e,a); stroke(&e,b); stroke(&e,c); stroke(&e,d);
        hold(&e,0,10); assert(e.good==1 && e.bad==0 && e.aborted==0);
        assert(e.last[0]==2 && e.last[1]==1 && e.last[2]==1); ++tested;
    }
    assert(tested==12);
    /* Independent multiset oracle over all words through length six. */
    unsigned words = 0;
    unsigned combinations = 1;
    for (unsigned length=0; length<=6; ++length) {
        for (unsigned code=0; code<combinations; ++code) {
            unsigned digits=code, expected[3]={0};
            setup(&e);
            for (unsigned i=0; i<length; ++i) {
                unsigned bin=digits%3; digits/=3;
                ++expected[bin]; stroke(&e,bin);
            }
            hold(&e,0,10);
            bool match=expected[0]==2 && expected[1]==1 && expected[2]==1;
            assert(e.good==(unsigned)match && e.bad==(unsigned)!match);
            for (unsigned i=0;i<3;++i) assert(e.last[i]==expected[i]);
            assert(engine_valid(&e)); ++words;
        }
        combinations*=3;
    }
    assert(words==1093);
    setup(&e); stroke(&e,0); hold(&e,0,10);
    assert(e.bad==1 && e.last[0]==1 && e.last_result==2);
    setup(&e); for (unsigned i=0;i<3;++i) stroke(&e,0);
    stroke(&e,1); stroke(&e,2); hold(&e,0,10);
    assert(e.bad==1 && e.good==0);
    setup(&e); hold(&e,9,100); assert(e.count[0]==1);
    hold(&e,8,10); hold(&e,9,4); hold(&e,8,10); assert(e.count[0]==1);
    /* A held paddle at docking cannot establish a new kit. */
    engine_init(&e); t=0; hold(&e,1,10); hold(&e,9,10); assert(e.starts==0);
    hold(&e,0,10); hold(&e,8,10); assert(e.starts==1);
    /* Gap abandons; high tray cannot rearm. */
    setup(&e); t+=5; engine_sample(&e,t,8);
    assert(e.aborted==1 && e.gaps==1 && e.state==WAIT);
    hold(&e,8,20); assert(e.starts==1); hold(&e,0,10); hold(&e,8,10);
    assert(e.starts==2);
    /* Tray removal tied to stroke is ambiguous, never a good kit. */
    setup(&e); stroke(&e,0); stroke(&e,0); stroke(&e,1);
    hold(&e,4,10); assert(e.bad==1 && e.good==0);
    /* Any raw tray drop during an active stroke taints the batch. */
    setup(&e); hold(&e,1,1); hold(&e,8,10);
    assert(e.tainted);
    setup(&e); hold(&e,8,60001); assert(e.aborted==1 && e.state==WAIT);
    setup(&e); stroke(&e,0); stroke(&e,0); stroke(&e,1); stroke(&e,2);
    e.age=59994; hold(&e,0,5); assert(e.good==1 && e.last_age==59999);
    setup(&e); stroke(&e,0); stroke(&e,0); stroke(&e,1); stroke(&e,2);
    e.age=59995; hold(&e,0,5); assert(e.aborted==1 && e.good==0 && e.last_age==60000);
    setup(&e); e.good=UINT32_MAX-1; e.starts=UINT32_MAX;
    e.bad=0; e.aborted=0;
    for(unsigned i=0;i<2;++i) stroke(&e,0);
    stroke(&e,1); stroke(&e,2);
    hold(&e,0,10); assert(e.good==UINT32_MAX && e.state==READY);
    hold(&e,8,10); assert(e.state==HALT && (e.faults & F_OVERFLOW));
    setup(&e); e.count[0]=UINT32_MAX; stroke(&e,0);
    assert(e.state==HALT && e.aborted==1);
    /* Independent debounce: noise on C must not mask A. */
    setup(&e); for(unsigned i=0;i<20;++i) hold(&e,9u|((i&1u)?4u:0u),1);
    assert(e.count[0]==1 && e.count[2]==0);
    setup(&e); char frame[768]; size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n>0 && strstr(frame,"KM1,")==frame && frame[n-1]=='\n');
    assert(wire_encode(&e,frame,5)==0);
    /* Timestamp wrap is normal, a skipped tick is not. */
    engine_init(&e); t=UINT32_MAX-4; hold(&e,0,10); assert(e.gaps==0);
    assert(e.state==READY);
    puts("PASS: 12 recipe permutations, 1093 oracle words, boundaries, faults, overflow, wire");
}
