/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine engine;
static uint32_t tick;
static void hold(unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(&engine, ++tick, (uint8_t)raw);
        assert(engine_valid(&engine));
        if (!(tick % 100)) {
            char frame[768];
            size_t n=wire_encode(&engine,frame,sizeof frame);
            assert(n); fwrite(frame,1,n,stdout);
        }
    }
}
int main(void)
{
    engine_init(&engine);
    hold(0,100); hold(4,10); hold(0,10);
    const unsigned q[]={1,3,2,0};
    for (unsigned trial=0;trial<3;++trial) {
        for (unsigned i=0;i<96;++i) {
            hold(q[i%4],10);
            if (i < 12-trial) {
                hold(q[i%4]|8,10);
                hold(q[i%4],10);
            }
        }
        hold(4,10); hold(0,10);
    }
    hold(1,10); hold(0,10); /* Reversal ends the fourth capture. */
    hold(0,200);
    assert(engine.good==3 && engine.bad==1 && engine.mismatch==2);
    return 0;
}
