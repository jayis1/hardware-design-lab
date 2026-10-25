/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static uint32_t now, sequence;
static void hold(uint8_t bits, unsigned duration)
{
    while (duration--) engine_sample(&e,bits,++now);
    char out[256];
    size_t count=wire_status(&e,sequence++,out,sizeof out);
    fwrite(out,1,count,stdout);
}
int main(void)
{
    engine_init(&e,0);
    hold(12,20); hold(4,120); hold(12,10);
    hold(8,10); hold(9,10); hold(11,10); hold(10,10); hold(8,10); hold(12,10);
    hold(4,120); hold(12,10);
    return 0;
}
