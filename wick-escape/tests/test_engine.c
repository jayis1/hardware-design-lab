/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void samples(Engine *e, unsigned mask, unsigned count)
{
    uint16_t values[4];
    for (unsigned i=0;i<4;i++) values[i]=(mask&(1u<<i))?500:4000;
    for (unsigned n=0;n<count;n++) { now+=20; engine_sample(e,now,values,true); }
}
static void ready(Engine *e)
{
    now=0; engine_init(e); samples(e,0,55); assert(e->state==READY);
}
int main(int argc,char **argv)
{
    Engine e;
    ready(&e); samples(&e,1,5); assert(e.state==ACTIVE);
    samples(&e,3,10); assert(e.state==COMPLETE); assert(e.first[1]==100);
    assert(e.first[0]==0); assert(e.faults==0);
    char frame[256]; size_t n=wire_encode(&e,7,frame,sizeof frame);
    assert(n && frame[n-1]=='\n'); assert(!wire_encode(&e,7,frame,8));
    if(argc==2) { FILE *f=fopen(argv[1],"w"); assert(f); n=wire_encode(&e,7,frame,sizeof frame); fwrite(frame,1,n,f); fclose(f); }
    samples(&e,7,5); assert(e.state==INVALID && (e.faults&F_BYPASS));
    ready(&e); samples(&e,5,5); assert(e.state==INVALID);
    ready(&e); samples(&e,3,5); assert(e.state==INVALID && (e.faults&F_ORDER));
    ready(&e); samples(&e,2,5); assert(e.state==INVALID);
    ready(&e); samples(&e,1,4); assert(e.state==READY);
    samples(&e,0,5); assert(e.state==READY);
    samples(&e,1,5); assert(e.state==ACTIVE);
    now+=100; uint16_t dry[4]={4000,4000,4000,4000};
    engine_sample(&e,now,dry,true); assert(e.state==INVALID && (e.faults&F_GAP));
    ready(&e); samples(&e,1,5); samples(&e,1,30000);
    assert(e.state==EXPIRED && e.elapsed==600000);
    ready(&e); now+=20; engine_sample(&e,now,dry,false);
    assert(e.state==INVALID && (e.faults&F_ADC));
    ready(&e); samples(&e,1,5); samples(&e,0,5);
    assert(e.state==INVALID && (e.faults&F_DRY));
    engine_init(&e); now=0xfffffc00u; samples(&e,0,55);
    assert(e.state==READY); samples(&e,1,5); samples(&e,3,5);
    assert(e.state==COMPLETE && e.first[1]==100);
    ready(&e); dry[0]=4096; now+=20; engine_sample(&e,now,dry,true);
    assert(e.state==INVALID);
    assert(wire_crc("123456789",9)==0x29b1);
    ready(&e); samples(&e,1,4);
    now+=40; uint16_t wet[4]={500,4000,4000,4000};
    engine_sample(&e,now,wet,true);
    assert(e.state==INVALID && (e.faults&F_GAP));
    ready(&e); samples(&e,1,5); samples(&e,3,5);
    e.wet=7; assert(!engine_valid(&e));
    e.wet=3; e.first[2]=e.elapsed; assert(!engine_valid(&e));
    puts("engine: timing, ordering, bypass, late bypass, timeout, gap, ADC, wrap, CRC passed");
    return 0;
}
