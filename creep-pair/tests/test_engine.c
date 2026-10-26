/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Engine e;
static uint32_t now;
static uint16_t adc[4];
static void step(unsigned count) {
    while (count--) { now += PERIOD_MS; engine_sample(&e,now,adc,true); }
}
static void reset(void) {
    engine_init(&e); now=0;
    adc[0]=1000; adc[1]=1100; adc[2]=4095; adc[3]=4095;
}
static void baseline(void) { reset(); step(60); assert(e.lane[0].state==READY); }
int main(int argc,char **argv) {
    assert(argc==2);
    assert(resistance(0)==0);
    assert(resistance(4095)==UINT32_MAX);
    baseline();
    adc[2]=40; step(5); assert(e.lane[0].state==LOADED);
    assert(e.lane[1].state==READY);
    adc[0]=1500; step(3000);
    assert(e.lane[0].milestones==7);
    assert(e.lane[0].hold_ms>=60000);
    adc[2]=4095; step(5); assert(e.lane[0].state==RECOVERY);
    adc[0]=1050; step(3000); assert(e.lane[0].state==DONE);
    assert(e.lane[0].recovery_ms==60000);
    assert(e.lane[0].residual>0);
    assert(engine_valid(&e));
    char frame[512]; size_t n=wire_encode(&e,1,frame,sizeof frame);
    assert(n && n<sizeof frame);
    FILE *f=fopen(argv[1],"w"); assert(f); fwrite(frame,1,n,f); fclose(f);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,1,frame,3)==0);
    uint32_t residual=e.lane[0].residual;
    adc[0]=1300; step(5); assert(e.lane[0].residual==residual);
    baseline(); adc[2]=40; step(4); assert(e.lane[0].state==READY);
    adc[2]=4095; step(5); assert(e.lane[0].state==READY);
    baseline(); adc[2]=40; step(5); step(10);
    adc[2]=4095; step(5); assert(e.lane[0].state==INVALID);
    baseline(); now+=100; step(1); assert(e.faults & F_GAP);
    assert(e.lane[0].state==INVALID);
    reset(); adc[0]=4095; step(1); assert(e.faults & F_RANGE);
    reset(); engine_sample(&e,20,adc,false); assert(!e.acquired);
    assert(e.faults & F_ADC);
    baseline(); adc[2]=40; step(5); step(3000); adc[2]=4095; step(5);
    adc[2]=40; step(5); assert(e.lane[0].state==INVALID);
    reset(); now=UINT32_MAX-500; step(60); assert(e.lane[0].state==READY);
    baseline(); adc[2]=40; step(5); step(9001);
    assert(e.lane[0].state==INVALID);
    baseline(); adc[2]=40; step(5); step(2998);
    adc[2]=4095; step(2); assert(e.lane[0].state==INVALID);
    baseline(); adc[2]=40; step(5); step(8996);
    adc[2]=4095; step(5); assert(e.lane[0].state==INVALID);
    reset(); engine_fault(&e,F_TX); assert(engine_valid(&e));
    puts("PASS engine: timing, independent lanes, faults, wrap, CRC, bounds");
    return 0;
}
