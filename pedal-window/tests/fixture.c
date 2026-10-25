/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static uint32_t now, seq;
static void hold(uint8_t mask, unsigned duration)
{
    for (unsigned i=0; i<duration; ++i) engine_sample(&e,++now,mask);
    char line[256];
    size_t n=wire_status(&e,seq++,line,sizeof line);
    fwrite(line,1,n,stdout);
}
int main(void)
{
    engine_init(&e,0);
    hold(P_REST,30);
    hold(KEY_A|P_DOWN,30);
    hold(KEY_B|P_DOWN,20);
    hold(KEY_B|P_REST,20);
    hold(KEY_B|P_DOWN,20);
    hold(P_REST,30);
    hold(KEY_A|P_DOWN,30);
    hold(P_REST,40);
    hold(KEY_B|P_REST,20);
    hold(KEY_B|P_DOWN,20);
    return 0;
}
