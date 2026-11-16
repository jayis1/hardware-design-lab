/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
int main(int argc, char **argv)
{
    Engine e; engine_init(&e);
    char frame[768];
    (void)argv;
    if (argc > 1) {
        /* Every ordered raw-vector transition, including forbidden codes,
         * held qualification windows and subqualification disturbances. */
        for (unsigned a=0;a<16;++a) {
            for (unsigned b=0;b<16;++b) {
                for (unsigned t=0;t<21;++t) {
                    unsigned mask=t<10 ? a : (t==10 ? (a^1u) : b);
                    engine_sample(&e,e.now+1,(uint8_t)mask);
                    assert(wire_encode(&e,frame,sizeof frame));
                    fputs(frame,stdout);
                }
            }
        }
        return 0;
    }
    /* Synthetic sequence visits every matrix cell, including both drawers. */
    const unsigned codes[]={0,1,3};
    for (unsigned a=0;a<3;++a) {
        for (unsigned b=0;b<3;++b) {
            for (unsigned t=0;t<100;++t)
                engine_sample(&e,e.now+1,(uint8_t)(codes[a]|(codes[b]<<2)));
            assert(wire_encode(&e,frame,sizeof frame));
            fputs(frame,stdout);
        }
    }
    engine_sample(&e,e.now+2,0);
    assert(wire_encode(&e,frame,sizeof frame)); fputs(frame,stdout);
    return 0;
}
