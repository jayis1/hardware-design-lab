/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic inputs through the production engine AND wire encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine e;
static unsigned now;
static void hold(unsigned raw,unsigned duration)
{
    while(duration--) {
        engine_sample(&e,++now,raw);
        if(now%10==0) {
            char frame[768];
            assert(wire_encode(&e,frame,sizeof frame));
            fputs(frame,stdout);
        }
    }
}
int main(void)
{
    engine_init(&e);
    hold(0,20); hold(9,20);
    hold(13,20); hold(9,20); hold(8,20);
    hold(10,20); hold(14,20); hold(10,20); hold(0,20);
    assert(e.good==1);
    hold(9,20); hold(13,20); hold(9,20);
    hold(13,20); hold(9,20); hold(0,20);
    assert(e.bad==1);
    hold(9,20); now+=2; engine_sample(&e,now,9);
    hold(9,20); hold(0,20); assert(e.aborted==1);
    return 0;
}
