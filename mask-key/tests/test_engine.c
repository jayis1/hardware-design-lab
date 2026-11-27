/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t tick;
static void settle(Engine *e, unsigned word)
{
    for (unsigned i=0; i<=HOLD_MS; ++i)
        engine_sample(e, ++tick, (uint8_t)word);
    assert(engine_valid(e));
}
/* Independent oracle: partition records into buckets, each with one label. */
static unsigned oracle(unsigned settings, unsigned mask)
{
    unsigned buckets[8]={0};
    for (unsigned w=0; w<16; ++w)
        if (settings & (1u<<w)) buckets[w & mask] |= 1u<<(w/8);
    for (unsigned i=0; i<8; ++i)
        if (buckets[i]==3) return 0;
    return 1;
}
int main(void)
{
    Engine e;
    for (unsigned settings=0; settings<65536; ++settings) {
        engine_init(&e); e.settings=settings; e.coverage=engine_project(settings);
        unsigned expected=0, minimal=0;
        for (unsigned mask=0; mask<8; ++mask) {
            if (oracle(settings,mask)) expected |= 1u<<mask;
            unsigned witness=engine_witness(&e,mask);
            if (oracle(settings,mask)) assert(witness==256);
            else {
                unsigned a=witness&15, b=witness>>4;
                assert(a<b && b<16);
                assert((settings&(1u<<a)) && (settings&(1u<<b)));
                assert(a/8 != b/8 && (a&mask)==(b&mask));
            }
        }
        for (unsigned mask=0; mask<8; ++mask) {
            if (!(expected & (1u<<mask))) continue;
            unsigned redundant=0;
            for (unsigned sub=0; sub<mask; ++sub)
                if ((sub & mask)==sub && (expected & (1u<<sub))) redundant=1;
            if (!redundant) minimal |= 1u<<mask;
        }
        assert(engine_subsets(&e)==expected);
        assert(engine_minimal(&e)==minimal);
        for (unsigned raw=0; raw<16; ++raw) {
            e.raw=raw;
            unsigned best=8, cost=4;
            for (unsigned input=0; input<8; ++input) {
                unsigned row=((settings>>input)&1) | (((settings>>(input+8))&1)<<1);
                assert(engine_row(&e,input)==row);
                unsigned x=input^(raw&7), d=(x&1)+((x>>1)&1)+((x>>2)&1);
                if (!row && d<cost) { best=input; cost=d; }
            }
            assert(engine_suggest(&e)==best);
        }
    }
    for (unsigned a=0; a<16; ++a) for (unsigned b=0; b<16; ++b) {
        tick=0; engine_init(&e); settle(&e,a); settle(&e,b);
        assert(e.settings==((1u<<a)|(1u<<b)));
        assert(e.visits==(a==b?1u:2u));
    }
    tick=0; engine_init(&e);
    engine_sample(&e,++tick,0);
    for (unsigned i=0;i<99;++i) engine_sample(&e,++tick,0);
    assert(!e.settings); engine_sample(&e,++tick,0); assert(e.settings==1);
    engine_sample(&e,tick+=3,0);
    assert(!e.credited && !e.hold && e.gaps==1 && e.missed==2);
    for (unsigned i=0;i<99;++i) engine_sample(&e,++tick,0);
    assert(e.visits==1); engine_sample(&e,++tick,0); assert(e.visits==2);
    settle(&e,8); assert(engine_subsets(&e)==0);
    unsigned history=e.settings; settle(&e,0); assert(e.settings==history);
    char frame[1024]; assert(wire_encode(&e,frame,sizeof frame));
    assert(strncmp(frame,"MK1,",4)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_encode(&e,frame,4));
    engine_init(&e); engine_sample(&e,UINT32_MAX,0); engine_sample(&e,0,0);
    assert(e.hold==1 && !e.gaps);
    e.samples=UINT32_MAX; Engine before=e;
    engine_sample(&e,1,0); assert(e.fault==F_EXHAUST);
    assert(e.samples==before.samples && e.hold==before.hold);
    engine_init(&e); engine_sample(&e,0,16); assert(e.fault==F_INPUT);
    engine_fault(&e,F_CONFIG); assert(e.fault==F_INPUT);
    puts("PASS all 65536 relations: subset/minimal/witness/row/guidance oracles; stable pairs, thresholds, gaps, wrap, exhaustion, CRC");
}
