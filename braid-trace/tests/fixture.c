/* Author: jayis1. SPDX-License-Identifier: MIT
 * Synthetic input fixture, not captured hardware measurements. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void emit(Engine *e)
{
    char frame[768];
    assert(wire_encode(e,frame,sizeof frame));
    fputs(frame,stdout);
}
static void ticks(Engine *e,unsigned raw,unsigned n)
{
    while(n--) engine_sample(e,e->now+1,(uint8_t)raw);
    emit(e);
}
int main(void)
{
    Engine e; engine_init(&e); ticks(&e,1,10);
    unsigned codes[]={1,2,4,3,1,1};
    for(unsigned i=0;i<6;++i) {
        unsigned pair=(codes[i]-1)%2+1;
        ticks(&e,pair,100);
        ticks(&e,pair+(codes[i]<=2?4:8),100);
        ticks(&e,pair,100);
    }
    assert(e.accepted==6 && e.depth==2 && e.permutation==21);
    return 0;
}
