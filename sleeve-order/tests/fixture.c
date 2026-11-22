/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <assert.h>
#include "engine.h"
#include "wire.h"
static void level(Engine *e, unsigned mask)
{
    for(unsigned i=0;i<8;i++) {
        char frame[768];
        engine_sample(e,e->now+1,mask);
        assert(wire_encode(e,frame,sizeof frame));
        fputs(frame,stdout);
    }
}
int main(void)
{
    Engine e;
    engine_init(&e);
    unsigned masks[]={5,4,6,2,10,2,6,4,5,4,6,4,5,4,6,2,0,5};
    for(unsigned i=0;i<sizeof masks/sizeof masks[0];i++) level(&e,masks[i]);
    return 0;
}
