/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic fixture: all eight settings of Y=A XOR B, then contradiction. */
#include <stdio.h>
#include <assert.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine e;
    engine_init(&e);
    uint32_t tick=0;
    char frame[1024];
    for (unsigned stage=0; stage<9; ++stage) {
        unsigned input=stage<8?stage:0;
        unsigned output=stage<8?((input&1)^((input>>1)&1)):1;
        for (unsigned i=0;i<200;++i) {
            engine_sample(&e,++tick,(uint8_t)(input+8*output));
            if (tick%25==0) {
                assert(wire_encode(&e,frame,sizeof frame));
                fputs(frame,stdout);
            }
        }
    }
    assert(engine_subsets(&e)==0);
}
