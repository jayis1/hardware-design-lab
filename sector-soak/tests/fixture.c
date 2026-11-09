/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static void emit(const Engine *e)
{
    char frame[768];
    size_t n=wire_encode(e,frame,sizeof frame);
    if (!n) exit(1);
    fwrite(frame,1,n,stdout);
}
int main(void)
{
    Engine e;
    engine_init(&e);
    unsigned t=0;
    /* Synthetic pointing sweep, reverse, skipped sector, then frozen fault. */
    const unsigned path[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,15,14,6};
    for (unsigned i=0;i<sizeof path/sizeof path[0];++i) {
        for (unsigned j=0;j<100;++j) {
            engine_sample(&e,++t,(uint8_t)(path[i]^(path[i]>>1)));
            if (j==49 || j==99) emit(&e);
        }
    }
    engine_sample(&e,t+2,0); emit(&e);
    engine_sample(&e,t+100,0); emit(&e);
    return 0;
}
