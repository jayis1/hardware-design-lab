/* Author: jayis1. SPDX-License-Identifier: MIT
 * Synthetic contact study, never a physical measurement. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static uint32_t tick;
static void hold(unsigned raw,unsigned n)
{
    while(n--) {
        engine_sample(&e,++tick,(uint8_t)raw);
        char out[768]; size_t length=wire_encode(&e,out,sizeof out);
        assert(length); fputs(out,stdout);
    }
}
static void press(unsigned id,unsigned key)
{
    unsigned raw=id^(id>>1);
    hold(raw,8); hold(raw|key,8); hold(raw,8);
}
int main(void)
{
    engine_init(&e); hold(0,8);
    press(1,4); press(0,8); /* A preferred to B */
    press(2,4); press(1,8); /* B preferred to C */
    press(0,4); press(2,8); /* C preferred to A closes cycle */
    hold(0,100);
    return 0;
}
