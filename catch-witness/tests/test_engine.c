/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void tick(Engine *e, unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(e,e->now+1,(uint8_t)raw);
        assert(engine_valid(e));
    }
}
static Engine ready(void)
{
    Engine e; engine_init(&e); tick(&e,0,60);
    assert(e.state==READY); return e;
}
static Engine latched(void)
{
    Engine e=ready(); tick(&e,3,210);
    assert(e.state==LATCHED && e.settled==1); return e;
}
int main(void)
{
    Engine e=latched(); tick(&e,0,5);
    assert(e.unexplained==1 && e.state==DONE);
    e=latched(); tick(&e,7,5); tick(&e,3,5); tick(&e,0,5);
    assert(e.intentional==1 && e.unexplained==0);
    e=latched(); tick(&e,11,5); tick(&e,3,5); tick(&e,0,5);
    assert(e.challenged==1);
    /* A handle tied to opening cannot establish prior release intent. */
    e=latched(); tick(&e,4,5);
    assert(e.unexplained==1 && e.intentional==0);
    /* Held handle credit expires; it is not renewed by the high level. */
    e=latched(); tick(&e,7,510); tick(&e,4,5);
    assert(e.unexplained==1);
    /* Expiration is inclusive at 500, expired at 501. */
    for (unsigned age=500; age<=501; ++age) {
        e=latched(); tick(&e,7,5); tick(&e,3,age-5); tick(&e,0,5);
        assert(e.intentional==(age==500));
    }
    /* A tug must follow an established LATCHED sample. A settling-phase
     * declaration, including the transition sample, is not a retention test. */
    e=ready(); tick(&e,3,200); tick(&e,11,5);
    assert(e.state==LATCHED && !e.tug_credit);
    tick(&e,0,5); assert(e.unexplained==1);
    e=ready(); tick(&e,1,5); tick(&e,0,5);
    assert(e.unseated==1 && e.settled==0);
    e=ready(); tick(&e,3,5); tick(&e,3,199);
    assert(e.state==SETTLING); tick(&e,3,1); assert(e.state==LATCHED);
    /* A one-sample raw loss resets settlement despite debounce filtering. */
    e=ready(); tick(&e,3,204); tick(&e,1,1); tick(&e,3,200);
    assert(e.state==SETTLING); tick(&e,3,1); assert(e.state==LATCHED);
    e=latched(); engine_sample(&e,e.now+2,3);
    assert(e.state==HALT && e.aborted==1 && e.gaps==1 && engine_valid(&e));
    e=latched(); tick(&e,3,MAX_TRIAL_MS);
    assert(e.timed_out==1 && e.state==DONE);
    e=latched(); tick(&e,15,5); tick(&e,0,5);
    assert(e.intentional==1); /* Handle association takes precedence. */
    /* Input chatter on unrelated TUG cannot postpone door qualification. */
    e=latched(); for (unsigned i=0;i<5;++i) tick(&e,(i&1)?8:0,1);
    assert(e.unexplained==1);
    e=latched(); e.tx_errors=UINT32_MAX; engine_tx_error(&e);
    assert(e.state==HALT && e.aborted==1 && engine_valid(&e));
    e=ready(); e.starts=UINT32_MAX; e.unseated=UINT32_MAX;
    tick(&e,3,5); assert(e.state==HALT && e.starts==UINT32_MAX);
    e=latched(); e.now=UINT32_MAX-2; e.stamp=e.now; tick(&e,3,5);
    assert(e.state==LATCHED && e.now==2);
    /* Every possible closure mask followed by every opening mask is valid. */
    for (unsigned a=0;a<16;++a) for (unsigned b=0;b<16;++b) {
        e=ready(); tick(&e,a,220); tick(&e,b,520);
        char out[768]; assert(wire_encode(&e,out,sizeof out)>0);
        assert(wire_encode(&e,out,4)==0);
    }
    /* Independent age/presence oracle across both associations, precedence,
     * and the complete expiry boundary, without reusing engine helpers. */
    for (unsigned mask=0;mask<4;++mask) for (unsigned delay=5;delay<=510;++delay) {
        e=latched();
        unsigned inputs=3|((mask&1)?4:0)|((mask&2)?8:0);
        tick(&e,inputs,5); tick(&e,3,delay-5); tick(&e,0,5);
        unsigned intent=(mask&1) && delay<=500;
        unsigned challenge=!intent && (mask&2) && delay<=500;
        assert(e.intentional==intent);
        assert(e.challenged==challenge);
        assert(e.unexplained==(!intent && !challenge));
    }
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS: retention, credits, boundary, glitch, gap, overflow, wrap, 256 masks");
}
