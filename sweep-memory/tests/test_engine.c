/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) engine_sample(e, ++tick, raw);
}
static unsigned phase(int p)
{
    const unsigned ring[4] = {0,1,3,2};
    return ring[(unsigned)p & 3u];
}
static void setup(Engine *e)
{
    tick = 0;
    engine_init(e);
    hold(e, 0, 10);
    assert(e->state == READY);
}
int main(void)
{
    Engine e;
    /* All ordered phase pairs, independently defined by a circular list. */
    for (int a=0;a<4;a++) for (int b=0;b<4;b++) {
        int expected = b==a ? 0 : b==(a+1)%4 ? 1 : a==(b+1)%4 ? -1 : 2;
        assert(engine_direction(phase(a),phase(b)) == expected);
    }
    for (int sign=-1;sign<=1;sign+=2) {
        for (unsigned pattern=0;pattern<16;pattern++) {
            setup(&e);
            for (int p=1;p<=16;p++) {
                unsigned contacts=(pattern >> ((unsigned)p%4))&3u;
                hold(&e,phase(sign*p)|(contacts<<2),7);
            }
            assert(e.state==RETURNING && e.position==16);
            for (int p=15;p>=0;p--) {
                unsigned contacts=p ? (pattern >> ((unsigned)p%4))&3u : 0;
                hold(&e,phase(sign*p)|(contacts<<2),7);
            }
            assert(e.completed==1 && e.abandoned==0 && e.state==DONE);
            assert(e.mismatch_x==0 && e.mismatch_y==0);
            assert(engine_valid(&e));
            char buf[1024];
            assert(wire_encode(&e,buf,sizeof buf)>0);
            assert(strncmp(buf,"SM1,",4)==0);
            assert(wire_encode(&e,buf,3)==0);
            uint32_t done=e.completed;
            hold(&e,15,100);
            assert(e.completed==done); /* one-shot evidence */
        }
    }
    setup(&e);
    for (int p=1;p<=16;p++) hold(&e,phase(p),7);
    for (int p=15;p>=0;p--) hold(&e,phase(p)|12u,7);
    assert(e.mismatch_x==65535u && e.mismatch_y==65535u);
    setup(&e);
    hold(&e,1,7); hold(&e,0,7);
    assert(e.state==REJECTED && e.reason==R_REVERSAL && e.abandoned==1);
    setup(&e); hold(&e,3,7);
    assert(e.state==REJECTED && e.reason==R_JUMP);
    setup(&e); hold(&e,1,7);
    engine_sample(&e,tick+2,1);
    assert(e.reason==R_GAP && e.abandoned==1 && e.gaps==1);
    setup(&e); hold(&e,1,7);
    hold(&e,1,60001);
    assert(e.reason==R_TIMEOUT && e.abandoned==1);
    /* Acquire real elapsed age before testing tied final qualification. */
    for (unsigned boundary=0;boundary<2;boundary++) {
        setup(&e);
        for (int p=1;p<=16;p++) hold(&e,phase(p),7);
        for (int p=15;p>=1;p--) hold(&e,phase(p),7);
        unsigned target=SWEEP_TIMEOUT-6u-boundary;
        while (e.age<target) hold(&e,phase(1),1);
        hold(&e,phase(0),6);
        assert(boundary ? e.state==DONE : e.reason==R_TIMEOUT);
        assert(engine_valid(&e));
    }
    setup(&e);
    for (int p=1;p<=17;p++) hold(&e,phase(p),7);
    assert(e.reason==R_OVERSHOOT && e.abandoned==1);
    setup(&e); hold(&e,1,7); e.samples=UINT32_MAX;
    engine_sample(&e,++tick,1);
    assert(e.state==HALT && e.abandoned==1 && engine_valid(&e));
    setup(&e); e.samples=UINT32_MAX;
    engine_sample(&e,++tick,0);
    assert(e.fault==F_OVERFLOW && e.state==HALT);
    setup(&e); engine_sample(&e,++tick,16);
    assert(e.fault==F_INPUT);
    setup(&e); engine_fault(&e,F_CONFIG);
    assert(e.state==HALT && engine_valid(&e));
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS: phase oracle, both directions, pattern retraces, mismatch masks, faults");
}
