/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic test input through the production engine AND wire encoder. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine engine;
static uint32_t now;
static void samples(unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(&engine,++now,(uint8_t)raw);
        if (now%100==0) {
            char frame[768];
            size_t n=wire_encode(&engine,frame,sizeof frame);
            if (!n) { fputs("encoding failed\n",stderr); return; }
            fwrite(frame,1,n,stdout);
        }
    }
}
int main(void)
{
    engine_init(&engine);
    samples(0,100);
    for (unsigned i=0;i<48;i++) {
        unsigned value=(i%8)^(i>=32 ? 1:0);
        samples(value<<1,25);
        samples((value<<1)|1,12);
        samples(value<<1,1);
    }
    samples(0,100);
    return engine.fault!=0;
}
