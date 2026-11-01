/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t t;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++t, raw);
        assert(engine_valid(e));
    }
}
static void init(Engine *e)
{
    t=0;
    engine_init(e);
    hold(e,0,10);
    assert(e->state==READY);
    hold(e,9,10);
    assert(e->state==ACTIVE);
}
static void stroke(Engine *e, unsigned face)
{
    hold(e,8|face,10);
    hold(e,12|face,10);
    hold(e,8|face,10);
}
int main(void)
{
    Engine e;
    /* Independent oracle: exact coverage of two different faces, either order.
     * Includes every face word of length zero through eight. */
    unsigned cases=0;
    for (unsigned n=0;n<=8;++n) {
        for (unsigned word=0;word<(1u<<n);++word) {
            init(&e);
            unsigned coverage=0;
            for(unsigned i=0;i<n;++i) {
                unsigned face=(word&(1u<<i))?2:1;
                hold(&e,8,10);
                stroke(&e,face);
                coverage|=face;
            }
            hold(&e,0,10);
            assert(e.good==(n==2 && coverage==3));
            assert(e.bad==!e.good);
            assert(e.last_strokes==n);
            ++cases;
        }
    }
    init(&e); hold(&e,13,4); hold(&e,12,1);
    assert(e.strokes==1 && e.faces==0 && e.tainted==1);
    init(&e); stroke(&e,1);
    hold(&e,14,10); /* face changes with press: no prior setup */
    hold(&e,10,10); hold(&e,0,10); assert(e.bad==1);
    init(&e); hold(&e,13,10); hold(&e,12,1); /* raw glitch while down */
    hold(&e,13,10); hold(&e,9,10); stroke(&e,2);
    hold(&e,0,10); assert(e.bad==1);
    init(&e); stroke(&e,1); stroke(&e,2);
    hold(&e,14,10); hold(&e,0,10); assert(e.bad==1);
    init(&e); stroke(&e,1); stroke(&e,2);
    engine_sample(&e,t+=2,10); assert(e.aborted==1 && e.gaps==1);
    hold(&e,10,10); assert(e.state==WAIT);
    hold(&e,0,10); assert(e.state==READY);
    init(&e); e.age=MAX_TRIAL_MS-5;
    hold(&e,0,5); assert(e.aborted==1 && e.good==0 && e.bad==0);
    init(&e); e.strokes=UINT32_MAX; e.faces=3; e.tainted=1;
    hold(&e,13,5); assert(e.state==HALT && e.aborted==1);
    init(&e); t=UINT32_MAX; e.stamp=t;
    hold(&e,9,1); assert(e.gaps==0);
    init(&e); engine_fault(&e,F_WATCHDOG);
    assert(e.state==HALT && e.aborted==1 && engine_valid(&e));
    char out[768]; assert(wire_encode(&e,out,sizeof out)>0);
    assert(wire_encode(&e,out,4)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    e.starts++; assert(!engine_valid(&e));
    engine_init(&e); hold(&e,13,10); assert(e.starts==0);
    printf("PASS: %u face-word oracle cases and boundary/fault/CRC regressions\n",cases);
    return 0;
}
