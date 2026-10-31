/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void step(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++now, (uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void setup(Engine *e, unsigned endpoint)
{
    engine_init(e); now=0; step(e, endpoint, 8);
    assert(e->state==READY && e->anchor==endpoint);
}
int main(int argc, char **argv)
{
    Engine e; char frame[768];
    if (argc>1 && !strcmp(argv[1], "fixture")) {
        engine_init(&e); now=0;
        unsigned masks[]={2,8,8,1,4,4,2};
        for (unsigned i=0;i<7;i++) {
            step(&e,masks[i],100);
            size_t n=wire_encode(&e,frame,sizeof frame);
            assert(n); fwrite(frame,1,n,stdout);
        }
        return 0;
    }
    /* Every valve category and both stroke directions. */
    for (unsigned origin=1;origin<=2;origin++) {
        for (unsigned valves=0;valves<4;valves++) {
            setup(&e,origin); step(&e,valves<<2,10);
            assert(e.state==ACTIVE && e.starts==1);
            uint32_t before=e.bins[(origin==1 ? 4:0)+valves];
            step(&e,valves<<2,20);
            assert(e.bins[(origin==1 ? 4:0)+valves]==before+20);
            step(&e,3-origin,6);
            assert(e.completed[origin==1]==1 && e.aborted==0);
            assert(e.state==READY && e.anchor==3-origin);
            assert(wire_encode(&e,frame,sizeof frame));
            assert(!wire_encode(&e,frame,5));
        }
    }
    setup(&e,1); step(&e,0,10); step(&e,1,6);
    assert(e.aborted==1 && e.completed[0]==0 && e.completed[1]==0);
    setup(&e,2); step(&e,8,10);
    uint32_t before=e.bins[2];
    step(&e,12,1); step(&e,8,1);
    assert(e.bins[2]==before); /* raw glitch cannot earn time */
    step(&e,8,1); assert(e.bins[2]==before+1);
    setup(&e,2); step(&e,8,10);
    engine_sample(&e,now+2,8);
    assert(e.state==HALTED && e.aborted==1 && e.gaps==1 && engine_valid(&e));
    setup(&e,2); step(&e,8,10); step(&e,3,1);
    assert(e.state==HALTED && (e.flags & F_ENDPOINTS) && e.aborted==1);
    setup(&e,2); step(&e,8,5);
    while (e.active_ms<MAX_STROKE_MS-1) step(&e,8,1);
    step(&e,8,1);
    assert(e.state==WAIT && e.aborted==1);
    /* Timeout must not immediately start again in the middle. */
    step(&e,8,10); assert(e.starts==1);
    step(&e,1,5); assert(e.state==READY);
    /* Transactional overflow: rollback bin update, then abandon once. */
    setup(&e,2); step(&e,8,10);
    e.bins[2]=UINT32_MAX;
    step(&e,8,1);
    assert(e.state==HALTED && e.aborted==1 && e.bins[2]==UINT32_MAX);
    setup(&e,2); now=UINT32_MAX-2; e.time=now;
    step(&e,2,8); assert(e.state==READY);
    assert(wire_crc("123456789",9)==0x29b1);
    engine_init(&e); engine_fault(&e,F_WATCHDOG);
    assert(engine_valid(&e) && wire_encode(&e,frame,sizeof frame));
    /* Exhaust every raw mask as a next observation from both origins.
     * Chatter is included; compare bin increments to the interval contract. */
    for (unsigned origin=1;origin<=2;origin++) {
        for (unsigned mask=0;mask<16;mask++) {
            setup(&e,origin); step(&e,0,8);
            for (unsigned i=0;i<8;i++) {
                Engine old=e;
                step(&e,mask,1);
                uint64_t growth=0;
                for (unsigned b=0;b<8;b++) growth+=e.bins[b]-old.bins[b];
                bool eligible=old.state==ACTIVE && e.state!=HALTED
                    && (old.raw&3)==0 && (e.raw&3)==0
                    && (old.levels&3)==0 && (e.levels&3)==0
                    && old.raw==old.levels && e.raw==e.levels
                    && old.levels==e.levels;
                assert(growth==(unsigned)eligible);
                assert(wire_encode(&e,frame,sizeof frame));
            }
        }
    }
    /* Long deterministic mixed trace checks serialization on every state. */
    uint32_t seed=12345;
    engine_init(&e); now=0;
    for (unsigned i=0;i<20000;i++) {
        seed=seed*1664525u+1013904223u;
        if (e.state==HALTED && i%31==0) { engine_init(&e); now=0; }
        step(&e,(seed>>24)&15u,1);
        assert(wire_encode(&e,frame,sizeof frame));
    }
    puts("PASS categories, directions, boundaries, gap, timeout, overflow, wrap, CRC");
}
