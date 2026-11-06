/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
/* Synthetic stable contact groups, NOT physical rack measurements. */
int main(void)
{
    Engine e;
    engine_init(&e);
    const unsigned masks[]={0,1,3,7,5,13,12,0,3,0};
    uint32_t now=0;
    for(unsigned j=0;j<sizeof masks/sizeof masks[0];++j) {
        for(unsigned k=0;k<10;++k) engine_sample(&e,++now,(uint8_t)masks[j]);
        char frame[768];
        size_t n=wire_encode(&e,frame,sizeof frame);
        assert(n); assert(fwrite(frame,1,n,stdout)==n);
    }
    return 0;
}
