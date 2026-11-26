/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t tick;
static void settle(Engine *e, unsigned mask)
{
    for (unsigned i=0; i<=HOLD_MS; ++i)
        engine_sample(e, ++tick, (uint8_t)mask);
    assert(engine_valid(e));
}
static unsigned oracle_row(unsigned settings, unsigned input)
{
    unsigned result=0;
    for (unsigned output=0; output<4; ++output)
        if (settings & (1u << (input+4*output))) result |= 1u<<output;
    return result;
}
int main(void)
{
    Engine e;
    /* Exhaustive independent mapping and function-enumeration oracle. */
    for (unsigned settings=0; settings<65536; ++settings) {
        engine_init(&e); e.settings=settings; e.coverage=engine_project(settings);
        unsigned count=0;
        for (unsigned table=0; table<256; ++table) {
            int compatible=1;
            for (unsigned i=0;i<4;++i) {
                unsigned row=oracle_row(settings,i);
                if (row && row != (1u<<((table>>(2*i))&3))) compatible=0;
            }
            count+=compatible;
        }
        assert(engine_candidates(&e)==count);
        for (unsigned i=0;i<4;++i) assert(engine_row(&e,i)==oracle_row(settings,i));
        for (unsigned raw=0;raw<16;++raw) {
            e.raw=raw;
            unsigned best=4, distance=3;
            for (unsigned i=0;i<4;++i) if (!oracle_row(settings,i)) {
                unsigned x=i^(raw&3); unsigned d=(x&1)+((x>>1)&1);
                if (d<distance) { best=i; distance=d; }
            }
            assert(engine_suggest(&e)==best);
        }
    }
    for (unsigned a=0;a<16;++a) for (unsigned b=0;b<16;++b) {
        tick=0; engine_init(&e); settle(&e,a); settle(&e,b);
        assert(e.settings==((1u<<a)|(1u<<b)));
        assert(e.visits==(a==b?1u:2u));
        assert(engine_valid(&e));
    }
    tick=0; engine_init(&e);
    engine_sample(&e,++tick,5);
    for (unsigned i=0;i<99;++i) engine_sample(&e,++tick,5);
    assert(!e.settings); engine_sample(&e,++tick,5);
    assert(e.settings==(1u<<5));
    engine_sample(&e,tick+=3,5);
    assert(!e.credited && !e.hold && e.gaps==1 && e.missed==2);
    for (unsigned i=0;i<99;++i) engine_sample(&e,++tick,5);
    assert(e.visits==1); engine_sample(&e,++tick,5); assert(e.visits==2);
    settle(&e,1); assert(engine_candidates(&e)==0); /* Same input, different output. */
    unsigned history=e.settings; settle(&e,5); assert(e.settings==history);
    char frame[1024]; assert(wire_encode(&e,frame,sizeof frame));
    assert(strncmp(frame,"TS1,",4)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_encode(&e,frame,4));
    engine_init(&e); engine_sample(&e,UINT32_MAX,0); engine_sample(&e,0,0);
    assert(e.hold==1 && !e.gaps);
    Engine before=e; e.samples=UINT32_MAX; before=e;
    engine_sample(&e,1,0); assert(e.fault==F_EXHAUST);
    assert(e.samples==before.samples && e.hold==before.hold);
    engine_init(&e); engine_sample(&e,0,16); assert(e.fault==F_INPUT);
    engine_fault(&e,F_CONFIG); assert(e.fault==F_INPUT);
    puts("PASS exhaustive relation/hypothesis oracle, all stable pairs, thresholds, gaps, conflict retention, wrap, faults, CRC");
}
