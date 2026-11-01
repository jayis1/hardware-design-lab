/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static uint32_t tick;
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) { engine_sample(e, ++tick, raw); assert(engine_valid(e)); }
}
static unsigned gray(unsigned index) { return index ^ (index >> 1); }
static void reset(Engine *e) { tick=0; engine_init(e); hold(e,0,6); }
static void declare(Engine *e, unsigned index, unsigned result)
{
    unsigned s=gray(index);
    hold(e,s,6); hold(e,s|result,6); hold(e,s,6);
}
int main(void)
{
    Engine e;
    /* Every three-declaration word, including repeat and contradiction. */
    for (unsigned word=0; word<512; ++word) {
        reset(&e); unsigned pass=0, refuse=0, w=word;
        for (unsigned n=0;n<3;++n) {
            unsigned a=w%8; w/=8;
            if (a&4) refuse |= 1u<<(a&3); else pass |= 1u<<(a&3);
            declare(&e,a&3,(a&4)?8:4);
            bool conflict=false;
            for(unsigned i=0;i<4;++i) for(unsigned j=0;j<4;++j)
                if ((pass&(1u<<i)) && (refuse&(1u<<j)) && i>=j) conflict=true;
            assert(e.pass==pass && e.refuse==refuse);
            assert(e.conflict==(unsigned)conflict && e.accepted==n+1);
        }
    }
    reset(&e); hold(&e,12,6); hold(&e,0,6);
    assert(e.rejected==1 && !e.accepted);
    reset(&e); hold(&e,4,6); hold(&e,5,1); hold(&e,0,6);
    assert(e.rejected==1 && !e.pass);
    reset(&e); hold(&e,4,6); hold(&e,12,1); hold(&e,0,6);
    assert(e.rejected==1);
    reset(&e); hold(&e,4,6); engine_sample(&e,tick+=2,4);
    assert(e.aborted==1 && engine_valid(&e));
    hold(&e,4,6); assert(e.state==WAIT); hold(&e,0,6);
    declare(&e,2,4); assert(e.pass==4 && e.accepted==1);
    reset(&e); hold(&e,4,QUALIFY); assert(e.state==ACTIVE);
    hold(&e,4,MAX_PRESS_MS-1); assert(e.state==ACTIVE);
    hold(&e,0,1); assert(e.aborted==1 && e.state==WAIT);
    reset(&e); e.starts=UINT32_MAX; e.accepted=UINT32_MAX; e.pass=1;
    hold(&e,4,6); assert(e.state==HALT && e.faults==F_OVERFLOW);
    reset(&e); tick=UINT32_MAX-3; e.stamp=tick;
    hold(&e,0,8); assert(!e.gaps);
    reset(&e); char out[768]; assert(wire_encode(&e,out,sizeof out));
    assert(!wire_encode(&e,out,8)); assert(wire_crc("123456789",9)==0x29b1);
    e.pass=8; e.refuse=1; assert(!engine_valid(&e));
    reset(&e); hold(&e,4,QUALIFY); e.age=MAX_PRESS_MS;
    assert(!engine_valid(&e));
    e.age=0; e.outcome=12; e.tainted=0; assert(!engine_valid(&e));
    puts("PASS: 512 declaration words, conflict oracle and boundary cases");
}
