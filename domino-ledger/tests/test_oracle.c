/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void ticks(Engine *e,unsigned raw,unsigned n)
{
    while(n--) engine_sample(e,e->now+1,(uint8_t)raw);
}
int main(void)
{
    unsigned checked=0;
    /* Independent per-station history, not a copy of the engine state machine.
     * Four arbitrary 5-sample plateaus include falls, returns, ties and chatter-free holds. */
    for(unsigned word=0;word<65536;++word) {
        Engine e; engine_init(&e); ticks(&e,15,505);
        unsigned previous[4]={1,1,1,1};
        unsigned first[4]={ABSENT,ABSENT,ABSENT,ABSENT};
        unsigned returned[4]={0,0,0,0};
        unsigned onset=ABSENT;
        unsigned maskword=word;
        for(unsigned step=0;step<4;++step) {
            unsigned raw=maskword&15; maskword>>=4;
            unsigned time=5*(step+1);
            for(unsigned i=0;i<4;++i) {
                unsigned up=(raw>>i)&1;
                if(previous[i] && !up && first[i]==ABSENT) {
                    first[i]=time;
                    if(onset==ABSENT) onset=time;
                }
                if(!previous[i] && up && first[i]!=ABSENT) returned[i]=1;
                previous[i]=up;
            }
            ticks(&e,raw,5);
            assert(engine_valid(&e));
            assert(e.starts==(onset!=ABSENT));
            for(unsigned i=0;i<4;++i) {
                assert(e.offset[i]==(first[i]==ABSENT?ABSENT:first[i]-onset));
                assert(((e.rebound>>i)&1)==returned[i]);
            }
            char frame[768]; assert(wire_encode(&e,frame,sizeof frame));
        }
        ++checked;
    }
    printf("PASS: %u four-plateau words against independent station histories\n",checked);
    return 0;
}
