/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    while (ticks--) {
        engine_sample(e, e->now + 1u, mask);
        assert(engine_valid(e));
    }
}
static void level(Engine *e, unsigned mask) { hold(e, mask, 8); }
static void ready(Engine *e) { engine_init(e); level(e, 5); assert(e->phase==READY); }
static void extend(Engine *e) { level(e,4); level(e,6); assert(e->phase==EXTENDED); }
static void inner(Engine *e) { level(e,2); level(e,10); level(e,2); level(e,6); }
static void retract(Engine *e) { level(e,4); level(e,5); }
int main(void)
{
    Engine e;
    char text[768];
    ready(&e); extend(&e); inner(&e); inner(&e); retract(&e);
    assert(e.completed==1 && e.nested==1 && e.inner_trips==2 && e.aborted==0);
    assert(e.last==RESULT_NESTED);
    extend(&e); retract(&e);
    assert(e.completed==2 && e.nested==1 && e.last==RESULT_OUTER);
    /* Every stable illegal departure from READY must not become success. */
    for (unsigned mask=0;mask<16;mask++) {
        ready(&e); level(&e,mask);
        if (mask!=5 && mask!=4) assert(e.completed==0 && e.phase==WAIT);
    }
    /* Outer return during inner movement, including same-sample departure. */
    ready(&e); extend(&e); level(&e,2); level(&e,0);
    assert(e.aborted==1 && e.violations==1);
    ready(&e); extend(&e); level(&e,0);
    assert(e.aborted==1);
    /* Endpoint skip is not a witnessed traverse. */
    ready(&e); level(&e,6); assert(e.aborted==1);
    ready(&e); extend(&e); level(&e,10); assert(e.aborted==1);
    /* One-sample guard loss aborts even if qualification rejects the glitch. */
    ready(&e); extend(&e); level(&e,2); hold(&e,0,1);
    assert(e.aborted==1);
    /* Acquisition loss, timer wrap and exact deadline precedence. */
    ready(&e); extend(&e); engine_sample(&e,e.now+2,6);
    assert(e.gaps==1 && e.aborted==1 && e.phase==WAIT && engine_valid(&e));
    ready(&e); e.now=0xfffffffeu; level(&e,5); assert(e.phase==READY);
    ready(&e); extend(&e); e.age=LIMIT-1; hold(&e,6,1);
    assert(e.timeouts==1 && e.aborted==1 && e.last==RESULT_TIMEOUT);
    /* Exhaustion must roll back the entire staged sample. */
    ready(&e); e.starts=UINT32_MAX; e.completed=UINT32_MAX;
    level(&e,4); assert(e.phase==HALT && e.starts==UINT32_MAX);
    assert(engine_valid(&e));
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,text,sizeof text)>0);
    assert(!wire_encode(&e,text,4));
    assert(strncmp(text,"SV1",3)==0);
    /* Independent oracle over repeated inner round trips and outer cycles. */
    for(unsigned rounds=0;rounds<12;rounds++) {
        ready(&e);
        unsigned expected=0;
        for(unsigned cycle=0;cycle<8;cycle++) {
            extend(&e);
            for(unsigned j=0;j<rounds;j++) { inner(&e); expected++; }
            retract(&e);
            assert(e.completed==cycle+1 && e.inner_trips==expected);
            assert(e.nested==(rounds ? cycle+1 : 0));
        }
    }
    puts("PASS engine sequences, guards, gaps, bounds, CRC and independent counts");
}
