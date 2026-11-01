/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine engine;
static unsigned tick;
static void samples(unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(&engine,++tick,raw);
        if (tick%100==0) {
            char line[512];
            assert(wire_encode(&engine,line,sizeof line));
            fputs(line,stdout);
        }
    }
}
int main(void)
{
    engine_init(&engine);
    samples(0,100);
    samples(1,600); /* A neutral B */
    samples(0,100);
    samples(3,600); /* B neutral C */
    samples(0,100);
    samples(6,600); /* A heavier C: remains feasible */
    samples(0,100);
    samples(10,600); /* A lighter C: retained contradiction */
    samples(0,100);
    samples(13,600); /* invalid simultaneous sensors */
    return 0;
}
