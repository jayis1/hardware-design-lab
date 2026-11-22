/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e,e->now+1,raw);
        assert(engine_valid(e));
    }
}
static unsigned mask(unsigned missing) { return 15u ^ (1u << missing); }
static void oracle_words(void)
{
    /* Independent list of missing-foot visits; exercise every length-five word. */
    for (unsigned word=0; word<1024; ++word) {
        Engine e; engine_init(&e);
        unsigned expected[16]={0}, visits[4]={0}, last=4, seeds=0, w=word;
        for (unsigned j=0;j<5;++j) {
            unsigned foot=w%4; w/=4;
            if (foot!=last) {
                ++visits[foot];
                if (last<4) ++expected[last*4+foot]; else ++seeds;
                last=foot;
            }
            /* Transit through all-four avoids a two-foot raw ambiguity. */
            hold(&e,15,8); hold(&e,mask(foot),8);
        }
        assert(e.seeds==seeds && e.anchor==last);
        for(unsigned i=0;i<16;++i) assert(e.edges[i]==expected[i]);
        for(unsigned i=0;i<4;++i) assert(e.visits[i]==visits[i]);
    }
}
static void topology_pairs(void)
{
    /* All raw before/after vectors, including every two-or-fewer guard. */
    for (unsigned a=0; a<16; ++a) {
        for (unsigned b=0; b<16; ++b) {
            Engine e; engine_init(&e);
            hold(&e,a,8);
            unsigned previous=e.anchor;
            hold(&e,b,8);
            unsigned kind=support_class(b);
            assert(e.qualified==b);
            if (kind<4) assert(e.anchor==kind);
            if (kind==5) assert(e.anchor==4);
            if (kind==4) assert(e.anchor==previous);
            unsigned total=0;
            for (unsigned i=0;i<16;++i) total+=e.edges[i];
            assert(total==(unsigned)(previous<4 && kind<4 && previous!=kind));
        }
    }
}
int main(void)
{
    Engine e; engine_init(&e);
    for (unsigned m=0;m<16;++m) {
        unsigned count=0; for(unsigned b=0;b<4;++b) count+=(m>>b)&1;
        unsigned c=support_class(m);
        assert(c==(count==4?4:count==3? (m==14?0:m==13?1:m==11?2:3):5));
    }
    hold(&e,14,5); assert(!e.valid && e.samples==5 && e.unknown==5);
    hold(&e,14,1); assert(e.valid && e.anchor==0 && e.visits[0]==1);
    assert(e.dwell[0]==0);
    hold(&e,14,1); assert(e.dwell[0]==1);
    hold(&e,15,8); hold(&e,7,8);
    assert(e.edges[3]==1 && e.diagonal==1 && e.adjacent==0);
    hold(&e,15,8); hold(&e,11,8);
    assert(e.edges[14]==1 && e.adjacent==1);
    /* A raw multi-foot loss breaks correlation even if too brief to qualify. */
    hold(&e,0,1); assert(e.anchor==4);
    hold(&e,14,8); assert(e.seeds==2 && e.edges[8]==0);
    /* All-four settlement boundary: endpoint is not a credited interval. */
    hold(&e,15,6); assert(e.settle==0);
    hold(&e,15,999); assert(e.anchor==0 && e.settle==999);
    hold(&e,15,1); assert(e.anchor==4 && e.settle==1000);
    hold(&e,14,8); assert(e.seeds==3);
    /* Gap restarts qualification and cannot bridge a rock. */
    engine_sample(&e,e.now+10,7); assert(e.missed==9 && !e.valid && e.anchor==4);
    hold(&e,7,6); assert(e.seeds==4);
    Engine before=e;
    engine_sample(&e,e.now,7); assert(e.fault==F_TIME);
    assert(e.samples==before.samples);
    hold(&e,15,10); assert(e.samples==before.samples && e.anchor==before.anchor);
    engine_init(&e); e.now=UINT32_MAX-2;
    hold(&e,15,8); assert(!e.fault);
    engine_init(&e); engine_sample(&e,1,16); assert(e.fault==F_INPUT && engine_valid(&e));
    engine_init(&e); e.samples=UINT32_MAX; e.unknown=UINT32_MAX;
    engine_sample(&e,1,15); assert(e.fault==F_OVERFLOW && e.samples==UINT32_MAX);
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(!wire_encode(&e,frame,4));
    assert(wire_crc("123456789",9)==0x29b1);
    /* Final legal increment followed by transactional exhaustion. */
    engine_init(&e); e.samples=UINT32_MAX-1; e.unknown=UINT32_MAX-1;
    hold(&e,15,1); assert(e.samples==UINT32_MAX && !e.fault);
    before=e; engine_sample(&e,e.now+1,15);
    assert(e.fault==F_OVERFLOW && e.samples==before.samples && e.unknown==before.unknown);
    /* Glitch on the very last potential settlement interval prevents expiry. */
    engine_init(&e); hold(&e,14,8); hold(&e,15,6); hold(&e,15,999);
    hold(&e,14,1); assert(e.anchor==0 && e.settle==0);
    hold(&e,15,6); assert(e.anchor==0 && e.settle==0);
    hold(&e,15,1000); assert(e.anchor==4);
    topology_pairs();
    oracle_words();
    puts("PASS Foot Rock engine: 1024 independent words, boundaries, gaps, faults, CRC");
}
