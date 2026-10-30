/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t t;
static void hold(Engine *e, unsigned mask, unsigned ms)
{
    for (unsigned i=0; i<ms; ++i) engine_sample(e, ++t, mask);
    assert(engine_valid(e));
}
static void ready(Engine *e)
{
    engine_init(e); t=0; hold(e, 8, 120); assert(e->state == READY);
}
static void loaded(Engine *e)
{
    ready(e); hold(e, 11, 120);
}
static void cycle(Engine *e)
{
    hold(e, 3, 20); hold(e, 7, 20); hold(e, 3, 20); hold(e, 11, 20);
}
int main(int argc, char **argv)
{
    Engine e;
    engine_init(&e); t=0; hold(&e,8,6);
    assert(e.empty_ms == 0 && e.state == WAIT);
    hold(&e,8,99); assert(e.empty_ms == 99 && e.state == WAIT);
    hold(&e,8,1); assert(e.state == READY);
    ready(&e); hold(&e, 11, 6);
    assert(e.seat_ms == 0); /* Qualification endpoint earns no preceding time. */
    hold(&e, 11, 99); assert(e.seat_ms == 99);
    hold(&e, 3, 6); assert(e.reason & R_EARLY);
    ready(&e); hold(&e, 11, 106); assert(e.seat_ms == 100);
    hold(&e, 3, 6); assert(!(e.reason & R_EARLY));
    loaded(&e); hold(&e, 10, 1); hold(&e, 11, 1);
    assert(e.seat_ms == 0); /* Raw recovery does not credit the low interval. */
    hold(&e, 11, 99); assert(e.seat_ms == 99);
    loaded(&e); hold(&e, 3, 20);
    uint32_t before_gap=e.elapsed;
    engine_sample(&e,t+=6,3);
    assert(e.last_duration == before_gap+6);
    loaded(&e); cycle(&e);
    assert(e.good == 1 && e.bad == 0 && e.starts == 1);
    assert(e.state == DRAIN && e.last_reason == 0);
    /* A second stroke without removing the stack is not another clean trial. */
    cycle(&e); assert(e.extra == 1 && e.starts == 1);
    hold(&e, 8, 120); hold(&e, 11, 120); cycle(&e); assert(e.good == 2);
    loaded(&e); hold(&e, 3, 20); hold(&e, 2, 1); hold(&e, 7, 20);
    hold(&e, 11, 20); assert(e.bad == 1 && (e.last_reason & R_LOST));
    ready(&e); hold(&e, 3, 20); hold(&e, 7, 20); hold(&e, 11, 20);
    assert(e.bad == 1 && (e.last_reason & R_EARLY));
    loaded(&e); hold(&e, 3, 20); hold(&e, 11, 20);
    assert(e.bad == 1 && (e.last_reason & R_NO_BOTTOM));
    loaded(&e); hold(&e, 3, 20); hold(&e, 7, 20); hold(&e, 3, 20);
    hold(&e, 7, 20); hold(&e, 11, 20);
    assert(e.bad == 1 && (e.last_reason & R_REPEAT));
    loaded(&e); hold(&e, 15, 20); hold(&e, 3, 20); hold(&e, 11, 20);
    assert(e.bad == 1); /* impossible home/bottom combination */
    loaded(&e); hold(&e, 3, 20); hold(&e, 3, 30000);
    assert(e.aborted == 1 && (e.last_reason & R_TIMEOUT));
    loaded(&e); hold(&e, 3, 20); engine_sample(&e, t+=6, 3);
    assert(e.aborted == 1 && e.gaps == 1 && e.state == WAIT);
    hold(&e, 11, 120); assert(e.state == WAIT);
    hold(&e, 8, 120); assert(e.state == READY);
    /* Raw release exactly during bottom qualification remains a loss. */
    loaded(&e); hold(&e, 3, 20); hold(&e, 7, 5); hold(&e, 6, 1);
    hold(&e, 11, 20); assert(e.bad == 1);
    /* Ordinary millisecond rollover is continuous. */
    ready(&e); e.now=t=UINT32_MAX-3; hold(&e, 8, 10); assert(e.gaps == 0);
    loaded(&e); hold(&e, 3, 20); e.extra=COUNT_LIMIT;
    hold(&e, 7, 1); assert(e.state == HALT && e.aborted == 1);
    assert(e.starts == e.good+e.bad+e.aborted);
    /* Sweep every initial input and every pair of settled input vectors. */
    for (unsigned a=0;a<16;++a) for(unsigned b=0;b<16;++b) {
        ready(&e); hold(&e,a,120); hold(&e,b,120); hold(&e,8,120);
    }
    char out[768]; loaded(&e); cycle(&e);
    assert(wire_encode(&e,out,sizeof out)>0);
    assert(!wire_encode(&e,out,8));
    assert(wire_crc("123456789",9)==0x29b1);
    if (argc>1 && strcmp(argv[1],"--fixture")==0) {
        loaded(&e);
        wire_encode(&e,out,sizeof out); fputs(out,stdout);
        cycle(&e); wire_encode(&e,out,sizeof out); fputs(out,stdout);
    } else puts("engine: state, boundary, gap, wrap, exhaustion and 256 transition tests passed");
    return 0;
}
