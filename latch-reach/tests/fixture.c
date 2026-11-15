/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic observations, not measurements from a physical latch. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine engine;
static void emit(void)
{
    char frame[768];
    size_t size=wire_encode(&engine,frame,sizeof frame);
    assert(size);
    assert(fwrite(frame,1,size,stdout)==size);
}
static void hold(unsigned raw, unsigned ticks)
{
    while(ticks--) {
        engine_sample(&engine,engine.now+1,(uint8_t)raw);
        assert(engine_valid(&engine));
    }
    emit();
}
int main(void)
{
    engine_init(&engine);
    hold(1,110);
    for (unsigned seen=0;seen<4;++seen) {
        for (unsigned latch=0;latch<2;++latch) {
            hold(1,110);
            hold(0,10);
            if (seen&1) hold(2,10);
            hold(0,10);
            if (seen&2) hold(4,10);
            hold(0,10);
            hold(1|latch*8,110);
        }
    }
    assert(engine.completed==8);
    return 0;
}
