/* Author: jayis1. SPDX-License-Identifier: MIT. Synthetic loom, not measurements. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static unsigned sequence;
static void hold(unsigned raw, unsigned duration)
{
    for (unsigned i=0;i<duration;++i) {
        engine_sample(&e,(uint8_t)raw,e.now+1);
        if (e.now%250u==0) {
            char out[256];
            size_t n=wire_status(&e,sequence++,out,sizeof out);
            fwrite(out,1,n,stdout);
        }
    }
}
int main(void)
{
    engine_init(&e,0);
    hold(1,500);
    hold(0,250);
    hold(2,250);
    hold(6,250);
    hold(10,250);
    hold(8,250);
    hold(9,250);
    hold(13,250);
    hold(9,250);
    hold(8,250);
    hold(10,250);
    hold(14,250);
    return 0;
}
