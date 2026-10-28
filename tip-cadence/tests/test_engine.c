/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "engine.h"
#include "wire.h"
static uint32_t now;
static void run(Engine *e, uint8_t raw, unsigned n)
{
    while (n--) engine_sample(e, ++now, raw);
}
static void tip(Engine *e, uint8_t raw)
{
    run(e, raw, 20);
    run(e, 0, 180);
}
int main(int argc, char **argv)
{
    (void)argv;
    Engine e;
    engine_init(&e); now=0;
    run(&e,0,20);
    tip(&e,3); tip(&e,3); tip(&e,3); tip(&e,3);
    assert(e.lane[0].tips==4 && e.lane[1].tips==4);
    assert(e.lane[0].intervals==3);
    assert(e.lane[0].sum[0]==400 && e.lane[0].sum[1]==200);
    assert(e.lane[0].minimum==200 && e.lane[0].maximum==200);
    assert(e.lane[0].jitter==0);
    if (argc>1) {
        char frame[512];
        size_t n=wire_encode(&e,frame,sizeof frame);
        assert(n); fwrite(frame,1,n,stdout);
        run(&e,0,50);
        n=wire_encode(&e,frame,sizeof frame);
        assert(n); fwrite(frame,1,n,stdout);
        return 0;
    }
    /* A maintenance contact invalidates only its lane, including raw glitches. */
    run(&e,4,1); run(&e,0,20);
    tip(&e,3);
    assert(e.lane[0].intervals==3 && e.lane[1].intervals==4);
    assert(e.lane[0].breaks==1);
    /* Sub-qualification pulses cannot create tips. */
    uint32_t before=e.lane[0].tips;
    run(&e,1,3); run(&e,0,200);
    assert(e.lane[0].tips==before);
    /* A gap discards an active pulse and both interval anchors. */
    run(&e,1,10);
    engine_sample(&e,now+=10,1);
    run(&e,0,20);
    assert(e.gaps==1 && e.lane[0].tips==before);
    assert(!e.lane[0].anchored && !e.lane[1].anchored);
    /* Stuck active must not become a delayed valid tip. */
    run(&e,1,2100); run(&e,0,20);
    assert(e.lane[0].tips==before);
    assert(e.lane[0].rejected>=1);
    /* Ordinary timer wrap is continuous. */
    engine_init(&e); now=UINT32_MAX-10;
    run(&e,0,20); tip(&e,1); tip(&e,1);
    assert(e.gaps==0 && e.lane[0].intervals==1);
    /* Transactional overflow: neither lane gets a half-applied sample. */
    engine_init(&e); now=0; run(&e,0,20); run(&e,3,20);
    e.lane[1].tips=UINT32_MAX;
    run(&e,0,5);
    assert(e.faults & F_OVERFLOW);
    assert(e.lane[0].tips==0 && e.lane[1].tips==UINT32_MAX);
    char small[4], frame[512];
    assert(!wire_encode(&e,small,sizeof small));
    assert(wire_encode(&e,frame,sizeof frame));
    assert(wire_crc("123456789",9)==0x29b1);
    const unsigned widths[] = {9,10,2000,2001};
    for (unsigned i=0;i<4;++i) {
        engine_init(&e); now=0; run(&e,0,20);
        run(&e,1,widths[i]); run(&e,0,5);
        assert(e.lane[0].tips == (unsigned)(i==1 || i==2));
        assert(engine_valid(&e));
    }
    const unsigned periods[] = {99,100,60000,60001};
    for (unsigned i=0;i<4;++i) {
        engine_init(&e); now=0; run(&e,0,20);
        run(&e,1,20); run(&e,0,periods[i]-20);
        run(&e,1,20); run(&e,0,5);
        assert(e.lane[0].intervals == (unsigned)(i==1 || i==2));
        assert(engine_valid(&e));
    }
    /* Independent raw-vector fuzz, including discontinuities and wrap. */
    engine_init(&e); now=UINT32_MAX-500; uint32_t random=17;
    for (unsigned i=0;i<100000;++i) {
        random=random*1664525u+1013904223u;
        now += (random%1000==0) ? 7 : 1;
        engine_sample(&e,now,(uint8_t)((random>>28)&15));
        assert(engine_valid(&e));
    }
    puts("PASS cadence, lane isolation, gap, timeout, wrap, overflow, CRC and bounds");
}
