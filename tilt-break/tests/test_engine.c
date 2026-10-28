/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static uint16_t v[4];
static void tick(Engine *e) { now+=20; engine_sample(e,now,v,true); }
static void repeat(Engine *e,unsigned n) { while(n--) tick(e); }
static void setup(Engine *e) {
    now=0; v[0]=1000; v[1]=40; v[2]=4095; v[3]=4095;
    engine_init(e); repeat(e,5);
    v[3]=40; repeat(e,55);
    assert(e->state==READY);
    v[3]=4095; repeat(e,3);
    v[3]=40; repeat(e,3);
    assert(e->state==RAMP);
    v[3]=4095; repeat(e,3);
}
int main(int argc,char **argv) {
    Engine e; setup(&e);
    v[0]=1010; repeat(&e,2);
    v[1]=4095; tick(&e); uint32_t onset=now;
    repeat(&e,2); assert(e.state==SLIDE);
    assert(e.release_ms==onset); assert(e.release_hi>=e.release_lo);
    repeat(&e,7); v[2]=40; repeat(&e,3);
    assert(e.state==DONE); assert(e.transit_ms==200);
    int32_t hi=e.release_hi; uint32_t transit=e.transit_ms;
    v[0]=1800; repeat(&e,5);
    assert(e.release_hi==hi && e.transit_ms==transit);
    char line[512]; assert(wire_encode(&e,7,line,sizeof line));
    assert(!wire_encode(&e,7,line,8));
    if(argc>1) { FILE *f=fopen(argv[1],"w"); assert(f);
        wire_encode(&e,7,line,sizeof line); fputs(line,f); fclose(f); }
    setup(&e); v[1]=4095; tick(&e); v[1]=40; repeat(&e,4);
    assert(e.state==RAMP); /* Rejected contact glitch. */
    v[2]=40; repeat(&e,3); assert(e.state==INVALID && (e.faults&F_ORDER));
    setup(&e); now+=100; tick(&e); assert(e.faults&F_GAP);
    setup(&e); engine_sample(&e,now+20,v,false); assert(!e.valid && (e.faults&F_ADC));
    setup(&e); v[0]=4095; tick(&e); assert(e.faults&F_RANGE);
    setup(&e); v[1]=4095; repeat(&e,3); repeat(&e,501);
    assert(e.state==INVALID && (e.faults&F_TIMEOUT));
    setup(&e); v[3]=40; repeat(&e,3); assert(e.faults&F_CANCEL);
    setup(&e); v[0]=1200; tick(&e); assert(e.faults&F_SPEED);
    setup(&e); v[0]=1010; repeat(&e,3); v[0]=1000; tick(&e);
    assert(e.faults&F_REVERSE);
    engine_init(&e); now=0xfffffff0u; v[0]=1000; v[1]=40; v[2]=4095; v[3]=4095;
    tick(&e); tick(&e); assert(!(e.faults&F_GAP));
    assert(wire_crc("123456789",9)==0x29b1);
    puts("engine: release bracket, transit, glitches, ordering, faults, wrap and CRC pass");
    return 0;
}
