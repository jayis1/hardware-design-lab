/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static void hold(unsigned mask, unsigned n)
{
    while (n--) engine_sample(&e,e.now+1,(uint8_t)mask);
    char buffer[512]; size_t count=wire_encode(&e,buffer,sizeof buffer);
    assert(count); fwrite(buffer,1,count,stdout);
}
int main(void)
{
    engine_init(&e);
    hold(8,260); hold(0,5); hold(1,5); hold(0,5);
    hold(3,5); hold(7,5); hold(7,250);
}
