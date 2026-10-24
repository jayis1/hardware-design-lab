/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned ms)
{
    for (unsigned i=0;i<ms;++i) engine_sample(e,(uint8_t)raw,e->now+1);
}
int main(void)
{
    Engine e;
    engine_init(&e,0);
    hold(&e,1,25); /* left dock establishes origin */
    assert(e.origin==1 && e.ready);
    hold(&e,0,25);
    hold(&e,2,25); /* right arrival */
    assert(e.pending && e.accepted==0);
    hold(&e,6,25); /* beat */
    assert(e.accepted==1 && !e.pending);
    hold(&e,10,25); /* release beat; change shed */
    hold(&e,8,25);
    hold(&e,9,25);
    hold(&e,13,25);
    assert(e.accepted==2 && e.rejected==0);
    hold(&e,9,25);
    hold(&e,8,25);
    hold(&e,10,25);
    hold(&e,14,25); /* repeated shed */
    assert(e.accepted==2 && e.rejected==1 && e.reason==REPEAT_SHED);
    hold(&e,10,25);
    hold(&e,14,25); /* no crossing */
    assert(e.reason==EMPTY_BEAT);
    hold(&e,3,25); /* both docks impossible */
    assert(!e.ready && e.reason==BOTH_DOCKS);
    hold(&e,1,25);
    assert(e.ready);
    hold(&e,0,25);
    hold(&e,2,25);
    engine_sample(&e,2,e.now+50); /* missing acquisition invalidates */
    assert(!e.pending && !e.ready && e.gaps==1);
    hold(&e,2,25);
    assert(e.ready);
    engine_init(&e,UINT32_MAX-15);
    hold(&e,1,25);
    assert(e.ready);
    hold(&e,0,5); hold(&e,1,5);
    assert(e.origin==1 && !e.departed);
    hold(&e,0,25); hold(&e,2,25);
    hold(&e,2,10001);
    assert(!e.pending && e.reason==LATE_BEAT);
    char out[256];
    size_t n=wire_status(&e,42,out,sizeof out);
    assert(n>0 && out[0]=='@' && out[n-1]=='\n');
    assert(wire_status(&e,42,out,8)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    engine_init(&e,0);
    hold(&e,1,25); hold(&e,0,25); hold(&e,8,25);
    hold(&e,10,25); hold(&e,14,25);
    assert(e.accepted==0); /* changing shed in flight is not a valid pick */
    engine_init(&e,0);
    hold(&e,1,25); hold(&e,8,25); hold(&e,10,25); hold(&e,14,25);
    assert(e.accepted==0); /* simultaneous shed/departure is ambiguous */
    engine_init(&e,0);
    hold(&e,1,25); hold(&e,0,25); hold(&e,10,25); hold(&e,14,25);
    assert(e.accepted==0); /* simultaneous shed/arrival */
    engine_init(&e,0);
    hold(&e,1,25); hold(&e,0,10050); hold(&e,2,25); hold(&e,6,25);
    assert(e.accepted==0); /* do not join unrelated movements minutes apart */
    puts("PASS engine sequence, bounce, gap, timeout, wrap, framing");
    return 0;
}
