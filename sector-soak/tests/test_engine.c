/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static uint8_t gray(unsigned n) { return (uint8_t)(n ^ (n >> 1)); }
static void hold(Engine *e, unsigned sector, unsigned n)
{
    while (n--) {
        engine_sample(e, ++tick, gray(sector));
        assert(engine_valid(e));
    }
}
static void reset(Engine *e) { tick=0; engine_init(e); }
int main(void)
{
    Engine e;
    for (unsigned n=0;n<16;++n) assert(engine_decode(gray(n))==n);
    reset(&e);
    hold(&e, 0, 10);
    assert(e.sequence==10 && e.dwell[0]==5 && e.unknown==4);
    assert(e.sector==0 && e.valid==1);
    hold(&e, 1, 10);
    assert(e.forward==1 && e.backward==0 && e.dwell[1]==5);
    assert(e.unknown==9 && e.longest[0]==5);
    hold(&e, 0, 10);
    assert(e.backward==1 && e.reversals==1);
    hold(&e, 8, 10);
    assert(e.skipped==1 && e.direction==0);
    hold(&e, 9, 10);
    assert(e.reversals==1 && e.forward==2);
    /* Every ordered sector pair has independently defined ring adjacency. */
    for (unsigned a=0;a<16;++a) for (unsigned b=0;b<16;++b) {
        reset(&e); hold(&e,a,10); hold(&e,b,10);
        unsigned delta=(b+16-a)%16;
        assert(e.forward==(delta==1));
        assert(e.backward==(delta==15));
        assert(e.skipped==(delta!=0 && delta!=1 && delta!=15));
    }
    /* A raw glitch splits continuous-run credit even when rejected. */
    reset(&e); hold(&e,0,10); hold(&e,1,1); hold(&e,0,5);
    assert(e.forward==0 && e.longest[0]==5 && e.dwell[0]==5);
    hold(&e,0,3); assert(e.dwell[0]==8 && e.run==3);
    /* Missing acquisition freezes evidence permanently; time may advance. */
    uint32_t before=e.sequence;
    engine_sample(&e,tick+2,0);
    assert(e.state==HALT && e.fault==F_GAP && e.sequence==before);
    Engine stopped=e;
    engine_sample(&e,tick+100,15);
    assert(e.sequence==stopped.sequence && e.dwell[0]==stopped.dwell[0]);
    assert(engine_valid(&e));
    reset(&e); tick=0xfffffffaU; hold(&e,15,10);
    assert(e.state==TRACK && e.dwell[15]==5);
    engine_tx_error(&e); assert(e.tx_errors==1);
    char out[768];
    size_t size=wire_encode(&e,out,sizeof out);
    assert(size && !memcmp(out,"SS1,",4));
    assert(wire_encode(&e,out,4)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    e.dwell[3]++; assert(!engine_valid(&e));
    reset(&e); engine_fault(&e,F_CONFIG); assert(engine_valid(&e));
    /* Counter exhaustion must preserve the last valid partition. */
    reset(&e); hold(&e,0,5);
    e.sequence=LIMIT; e.unknown=LIMIT-1;
    assert(engine_valid(&e)); hold(&e,0,1);
    assert(e.fault==F_COUNTER && e.sequence==LIMIT && engine_valid(&e));
    puts("PASS Gray decode, 256 transitions, dwell, chatter, gaps, wrap, exhaustion and CRC");
}
