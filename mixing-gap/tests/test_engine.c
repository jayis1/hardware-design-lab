/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void qualify(Engine *e, unsigned word)
{
    engine_sample(e,e->now+1,(uint8_t)word);
    for (unsigned i=0;i<100;++i) engine_sample(e,e->now+1,(uint8_t)word);
    assert(engine_valid(e));
}
int main(void)
{
    for (unsigned a=0;a<16;++a) {
        for (unsigned b=0;b<16;++b) {
            Engine e; engine_init(&e);
            qualify(&e,a); qualify(&e,b);
            assert(e.settings==((1u<<a)|(1u<<b)));
            assert(e.visits==(a==b?1u:2u));
        }
    }
    Engine e; engine_init(&e);
    engine_sample(&e,10,3);
    for (unsigned t=11;t<110;++t) engine_sample(&e,t,3);
    assert(!e.visits);
    engine_sample(&e,110,3); assert(e.visits==1);
    engine_sample(&e,111,1); engine_sample(&e,112,3);
    assert(!e.hold && e.visits==1);
    engine_sample(&e,117,3);
    assert(e.gaps==1 && e.missed==4 && !e.hold);
    qualify(&e,1);
    char frame[1024];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,frame,sizeof frame));
    assert(strncmp(frame,"MG1,",4)==0);
    assert(!wire_encode(&e,frame,4));
    assert(!wire_encode(&e,NULL,sizeof frame));
    Engine before=e;
    e.samples=UINT32_MAX;
    engine_sample(&e,e.now+1,2);
    assert(e.fault==F_EXHAUST && e.settings==before.settings);
    engine_init(&e); e.now=UINT32_MAX-50; e.seeded=1; e.samples=1;
    qualify(&e,0); assert(e.visits==1 && engine_valid(&e));
    e.tx_errors=UINT32_MAX; engine_tx_error(&e); assert(e.tx_errors==UINT32_MAX);
    engine_fault(&e,F_CONFIG); engine_fault(&e,F_INPUT); assert(e.fault==F_CONFIG);
    before=e; engine_sample(&e,e.now+1,15);
    assert(e.samples==before.samples && e.settings==before.settings);
    engine_init(&e); engine_sample(&e,1,16); assert(e.fault==F_INPUT);
    puts("PASS: 256 ordered word pairs, qualification, gap, wrap, CRC, faults");
}
