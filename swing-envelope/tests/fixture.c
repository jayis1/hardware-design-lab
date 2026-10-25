/* Author: jayis1. SPDX-License-Identifier: MIT
 * Synthetic switch sequence, not a recording of a physical pendulum. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static uint32_t now, sequence;
static void hold(uint8_t mask, unsigned ms)
{
    while (ms--) {
        engine_sample(&e,mask,++now);
        if (now%250==0) {
            char line[256];
            size_t n=wire_status(&e,sequence++,line,sizeof line);
            fwrite(line,1,n,stdout);
        }
    }
}
static void pulse(uint8_t mask) { hold(mask,20); hold(0,80); }
int main(void)
{
    engine_init(&e,0);
    hold(0,250);
    for (unsigned i=0;i<8;++i) {
        pulse(1); pulse(2);
        if (i<5) pulse(8); else hold(0,100);
        hold(0,200);
        pulse(2); pulse(1);
        if (i<3) pulse(4); else hold(0,100);
        hold(0,200);
    }
    hold(0,250);
}
