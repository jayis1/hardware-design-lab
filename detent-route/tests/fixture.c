/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static void output(void)
{
    char frame[768]; size_t n=wire_encode(&e,frame,sizeof frame);
    if(n) fwrite(frame,1,n,stdout);
}
static void hold(unsigned mask,unsigned n)
{
    while(n--) engine_sample(&e,e.now+1,(uint8_t)mask);
    output();
}
int main(void)
{
    engine_init(&e); hold(1,6);
    unsigned route[]={1,2,3,0,3,3,1};
    for(unsigned i=0;i<sizeof route/sizeof route[0];i++) {
        hold(0,10); hold(1u<<route[i],10);
    }
    hold(0,10); hold(3,10); hold(8,10);
    return 0;
}
