/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static Engine e;
static uint32_t tick;
static void hold(uint8_t bits, unsigned ms)
{
    while (ms--) engine_sample(&e,bits,++tick);
}
static void init(void)
{
    tick=0;
    engine_init(&e,tick);
    hold(12,20); /* A=B=0, registration and tool parked closed. */
    assert(e.known && e.ready);
}
static void forward(void)
{
    hold(8,10); hold(9,10); hold(11,10); hold(10,10); hold(8,10);
    hold(12,10);
}
static void episode(void)
{
    hold(4,120); hold(12,10);
}
int main(void)
{
    assert(engine_reset_allows_home(1u<<26));
    assert(!engine_reset_allows_home((1u<<26)|(1u<<29)));
    assert(!engine_reset_allows_home((1u<<26)|(1u<<27)));
    assert(!engine_reset_allows_home(0));
    for (unsigned cause=25; cause<32; ++cause) {
        if (cause==26) continue;
        tick=0; engine_init(&e,0);
        e.home_attempted=!engine_reset_allows_home((1u<<26)|(1u<<cause));
        hold(12,20); episode();
        assert(!e.known && e.rejected==1 && e.accepted==0);
    }
    init(); hold(4,60020); hold(12,10);
    assert(e.overdue && e.rejected==1);
    init(); hold(4,10); e.episode_ms=UINT32_MAX-1;
    hold(4,10); assert(e.episode_ms==UINT32_MAX);
    init(); episode();
    assert(e.accepted==1 && e.station_counts[0]==1);
    forward(); assert(e.position==4); episode();
    assert(e.station_counts[1]==1 && e.accepted==2 && e.skips==0);
    episode(); assert(e.repeats==1);
    forward(); forward(); episode(); assert(e.skips==1);
    init(); hold(0,20); hold(12,20); assert(e.rejected==1);
    init(); hold(4,20); hold(5,10); hold(4,10); hold(12,20);
    assert(e.rejected==1 && e.motion_faults>=1);
    init(); hold(15,10); assert(!e.known && e.quad_faults==1);
    hold(12,10); episode(); assert(e.accepted==0 && e.rejected==1);
    init(); hold(4,20); tick+=10; engine_sample(&e,4,tick);
    hold(12,20); assert(!e.known && e.gaps==1 && e.rejected==1);
    init(); hold(4,2); hold(12,20); assert(!e.accepted && !e.rejected);
    init(); hold(0,10); hold(4,10); hold(12,10);
    assert(e.rejected==1); /* simultaneous registration and pickup fails */
    init(); hold(8,10); hold(10,10); hold(11,10); hold(9,10); hold(8,10); hold(12,10);
    assert(e.position==12); episode(); assert(e.station_counts[3]==1);
    engine_init(&e,UINT32_MAX-20); tick=UINT32_MAX-20;
    hold(12,30); episode(); assert(e.accepted==1 && !e.gaps);
    init(); e.accepted=UINT32_MAX; episode(); assert(e.accepted==UINT32_MAX);
    char out[256];
    assert(wire_crc("123456789",9)==0x29b1);
    size_t n=wire_status(&e,7,out,sizeof out); assert(n && out[n-1]=='\n');
    assert(!wire_status(&e,7,out,8));
    assert(!wire_status(NULL,7,out,sizeof out));
    puts("PASS engine: indexing, episodes, faults, gaps, wrap, CRC, capacity");
}
