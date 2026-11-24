/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic contacts only: this is not a physical measurement recording. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine e;
static void sample(unsigned raw)
{
    char frame[1024];
    engine_sample(&e,e.now+1,raw);
    size_t n=wire_encode(&e,frame,sizeof frame);
    assert(n); assert(fwrite(frame,1,n,stdout)==n);
}
static void hold(unsigned raw,unsigned n)
{
    while(n--) sample(raw);
}
int main(void)
{
    engine_init(&e);
    unsigned sequence[]={2,0,2,3,1,0,1,3,2};
    for(unsigned j=0;j<sizeof sequence/sizeof sequence[0];++j) {
        hold(0,6); hold(1u<<sequence[j],6);
    }
    hold(0,6); hold(1,6); hold(3,1); hold(0,6);
    hold(4,6);
    engine_sample(&e,e.now+8,0);
    hold(0,6);
    return 0;
}
