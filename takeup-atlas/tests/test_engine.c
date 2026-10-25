/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static Engine e;
static unsigned a, b;
static uint32_t tick;
static const unsigned phase[] = {0,1,3,2};
static void step(int in, int out)
{
    a = (a + in + 4) % 4;
    b = (b + out + 4) % 4;
    engine_sample(&e, ++tick, phase[a] | (phase[b] << 2));
}
static void fresh(void)
{
    a = b = tick = 0;
    engine_init(&e, 0, 0);
}
static void prime(void)
{
    for (int i=0; i<3; ++i) step(1,1);
    assert(e.state == TRACKING);
}
int main(void)
{
    fresh(); prime();
    step(-1,0); step(-1,0); step(-1,-1);
    assert(e.events == 0 && e.state == TAKING_UP);
    step(-1,-1); step(-1,-1);
    assert(e.events == 1 && e.first == 3 && e.confirm == 5);
    assert(e.last_direction == -1 && e.duration == 5);
    assert(e.state == TRACKING);
    char frame[256];
    size_t n = wire_encode(&e, 0, 0, frame, sizeof frame);
    assert(n > 0); fwrite(frame, 1, n, stdout);
    assert(wire_encode(&e,0,0,frame,8)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    fresh(); prime();
    step(-1,-1); step(-1,-1); step(-1,-1);
    assert(e.first == 1 && e.confirm == 3);
    /* Output leading the reversal is not credited to the new direction. */
    fresh(); prime(); step(0,-1); step(-1,0);
    assert(e.events == 0 && e.first == 0);
    step(0,-1); step(0,-1); step(0,-1);
    assert(e.events == 1 && e.first == 1 && e.confirm == 1);
    fresh(); prime(); step(-1,0); step(1,1);
    assert(e.rejected == 1 && e.events == 0 && e.state == PRIMING);
    fresh(); prime(); step(-1,-1); step(-1,1);
    assert(e.rejected == 1 && e.events == 0);
    fresh(); prime(); step(-1,0);
    for (unsigned i=0; i<10000; ++i) step(0,0);
    assert(e.rejected == 1 && e.state == PRIMING);
    fresh(); prime(); step(-1,0);
    tick += 2; engine_sample(&e,tick,phase[a] | (phase[b]<<2));
    assert(e.gaps == 1 && e.state == FAULT && e.rejected == 1);
    for (int i=0;i<20;++i) step(-1,-1);
    assert(e.events == 0 && e.state == FAULT);
    fresh(); engine_sample(&e,1,3);
    assert(e.axis[0].invalid == 1 && e.state == FAULT);
    fresh(); e.axis[0].position=INT32_MAX; step(1,0);
    assert(e.saturated && e.state == FAULT);
    fresh(); e.time=UINT32_MAX; tick=UINT32_MAX;
    step(1,1); assert(e.gaps==0 && e.axis[0].position==1);
    fresh(); prime(); e.events=UINT32_MAX;
    step(-1,-1); step(-1,-1); step(-1,-1);
    assert(e.saturated && e.events==UINT32_MAX && e.state==FAULT);
    puts("takeup engine tests passed");
    return 0;
}
