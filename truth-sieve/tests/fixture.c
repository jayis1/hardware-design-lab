/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic evidence only: enumerate a two-output half-adder, then conflict. */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine e;
    engine_init(&e);
    char frame[1024];
    unsigned now=0;
    for (unsigned input=0;input<5;++input) {
        unsigned a=input&1, b=(input>>1)&1;
        unsigned raw=input==4 ? 4 : (input | ((a^b)<<2) | ((a&b)<<3));
        for (unsigned i=0;i<220;++i) {
            engine_sample(&e,++now,raw);
            if (now%20==0) {
                assert(wire_encode(&e,frame,sizeof frame));
                fputs(frame,stdout);
            }
        }
    }
    assert(engine_candidates(&e)==0);
}
