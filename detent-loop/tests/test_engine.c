/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static uint16_t raw[4];
static FILE *recording;
static uint32_t sequence;
static void tick(Engine *e, unsigned count)
{
    while(count--) {
        now += 20;
        engine_sample(e, now, raw, true);
        if (recording && now % 200 == 0) {
            char line[512];
            assert(wire_encode(e, sequence++, line, sizeof line));
            fputs(line, recording);
        }
    }
}
static void button(Engine *e,unsigned i,bool closed)
{
    raw[i]=closed?40:4095;
    tick(e,3);
}
static void setup(Engine *e)
{
    engine_init(e); now=0;
    raw[0]=1200; raw[1]=1200; raw[2]=4095; raw[3]=4095;
    tick(e,5); assert(e->state==WAIT);
    button(e,2,true); tick(e,50); assert(e->state==READY);
    button(e,2,false); button(e,2,true); assert(e->state==OUT);
    button(e,2,false);
}
static void travel(Engine *e,unsigned value)
{
    while(raw[0]!=value) {
        raw[0] += raw[0]<value?1:-1;
        tick(e,1);
    }
}
int main(int argc,char **argv)
{
    Engine e;
    if (argc > 1) {
        recording = fopen(argv[1], "w");
        assert(recording);
    }
    setup(&e);
    raw[1]=1250;
    travel(&e,1450);
    assert(e.x_um>=5000 && e.peak_out_mn>0);
    button(&e,3,true); assert(e.state==BACK);
    button(&e,3,false);
    raw[1]=1150;
    travel(&e,1200); raw[1]=1200; tick(&e,1);
    button(&e,2,true); assert(e.state==DONE);
    assert(e.out_nj2>0 && e.back_nj2>0);
    int64_t work=e.out_nj2;
    tick(&e,10); assert(e.out_nj2==work);
    char frame[512];
    assert(wire_encode(&e,7,frame,sizeof frame)>0);
    assert(!wire_encode(&e,7,frame,4));
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,7,frame,sizeof frame)>0);
    if (recording) {
        fclose(recording);
        recording = NULL;
    }
    setup(&e); button(&e,3,true);
    assert(e.state==INVALID && (e.faults&F_SEQUENCE));
    setup(&e); travel(&e,1300); travel(&e,1260);
    assert(e.state==INVALID && (e.faults&F_DIRECTION));
    setup(&e); now+=100; tick(&e,1);
    assert(e.state==INVALID && (e.faults&F_GAP));
    setup(&e); engine_sample(&e,now+20,raw,false);
    assert(e.state==INVALID && !e.acquired && (e.faults&F_ADC));
    setup(&e); raw[0]=4095; tick(&e,1);
    assert(e.state==INVALID && !e.acquired && (e.faults&F_RANGE));
    setup(&e); raw[1]=2000; tick(&e,1);
    assert(e.state==INVALID && (e.faults&F_RANGE));
    setup(&e); raw[0]=1450; tick(&e,1);
    assert(e.state==INVALID && (e.faults&F_SPEED));
    setup(&e); tick(&e,6001);
    assert(e.state==INVALID && (e.faults&F_TIMEOUT));
    engine_init(&e); now=0xfffffff0u;
    raw[0]=1200;raw[1]=1200;raw[2]=raw[3]=4095;
    tick(&e,5); assert(!e.faults);
    engine_init(&e); now=0; tick(&e,5);
    button(&e,2,true); tick(&e,5); button(&e,2,false);
    assert(e.state==WAIT);
    setup(&e); travel(&e,1450); button(&e,3,true);
    button(&e,3,false); button(&e,2,true);
    assert(e.state==INVALID && (e.faults&F_SEQUENCE));
    /* Independent constant-force rectangle oracle for each signed leg. */
    setup(&e); raw[1]=1250; tick(&e,1);
    int32_t forward_force=e.force_mn;
    travel(&e,1450);
    assert(e.out_nj2==2*(int64_t)forward_force*e.x_um);
    int32_t endpoint=e.x_um;
    button(&e,3,true); button(&e,3,false);
    raw[1]=1150; tick(&e,1);
    int32_t return_force=e.force_mn;
    travel(&e,1200);
    assert(e.back_nj2==-2*(int64_t)return_force*endpoint);
    raw[1]=1200;tick(&e,1);button(&e,2,true);
    assert(e.state==DONE);
    int32_t frozen_x=e.x_um;
    raw[0]=1250;tick(&e,1);assert(e.x_um==frozen_x);
    engine_fault(&e,F_TX);
    assert(wire_encode(&e,9,frame,sizeof frame));
    /* Raw release interrupts zeroing even before button debounce completes. */
    engine_init(&e);now=0;raw[0]=raw[1]=1200;raw[2]=raw[3]=4095;
    tick(&e,3);button(&e,2,true);tick(&e,48);
    assert(e.state==ZERO);
    raw[2]=4095;tick(&e,1);assert(e.state==WAIT);
    /* Stable zero cannot be earned by a drifting sensor. */
    button(&e,2,false);button(&e,2,true);tick(&e,20);
    raw[0]+=20;tick(&e,1);assert(e.zero_count==0);
    assert(e.state==ZERO);
    /* A one-sample TURN glitch cannot prematurely change the active leg. */
    setup(&e);raw[3]=40;tick(&e,1);raw[3]=4095;tick(&e,3);
    assert(e.state==OUT);
    puts("PASS: loop integration oracle, frozen results, CRC, bounds, faults, wrap, zero and sequencing");
    return 0;
}
