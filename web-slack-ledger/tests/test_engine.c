/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void step(Engine *e, unsigned raw) { engine_sample(e, ++now, raw); }
static void forward(Engine *e) { step(e,1); step(e,3); step(e,2); step(e,0); }
int main(void)
{
    Engine e;
    engine_init(&e,0,0);
    now=0;
    forward(&e);
    assert(e.axis[0].position==4 && e.axis[1].position==0);
    assert(engine_difference(&e)==4);
    step(&e,2); step(&e,3); step(&e,1); step(&e,0);
    assert(e.axis[0].position==0);
    assert(e.reversals==1 && e.leg_peak==4);
    step(&e,3);
    assert(e.axis[0].invalid==1 && !engine_valid(&e));
    engine_init(&e,0,0);
    engine_sample(&e,5,1);
    assert(e.gaps==1 && e.axis[0].position==0 && !engine_valid(&e));
    engine_init(&e,UINT32_MAX,0);
    engine_sample(&e,0,1);
    assert(e.gaps==0 && e.axis[0].position==1);
    engine_init(&e,0,0);
    e.axis[0].position=INT32_MAX;
    engine_sample(&e,1,1);
    assert(e.axis[0].position==INT32_MAX && e.saturated);
    engine_init(&e,0,0);
    e.axis[0].position=INT32_MIN; e.axis[1].position=INT32_MAX;
    assert(engine_difference(&e)==-4294967295LL);
    char buffer[256];
    size_t n=wire_encode(&e,0,0,buffer,sizeof buffer);
    assert(n>0 && n<sizeof buffer && buffer[n-1]=='\n');
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,0,0,buffer,10)==0);
    engine_init(&e,0,0);
    now=0;
    for (unsigned i=0;i<10000;i++) {
        step(&e,5); step(&e,15); step(&e,10); step(&e,0);
    }
    assert(e.axis[0].position==40000 && engine_difference(&e)==0);
    assert(engine_valid(&e));
    step(&e,10); /* Both rollers reverse together; differential remains zero. */
    assert(e.origin==0 && e.leg_min==0 && e.leg_max==0);
    puts("engine: forward/reverse, diagonal, gaps, wrap, saturation, CRC, bounds, matched rollers PASS");
    n=wire_encode(&e,42,0,buffer,sizeof buffer);
    fwrite(buffer,1,n,stdout);
    return 0;
}
