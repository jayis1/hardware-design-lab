/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, unsigned mask, unsigned count)
{
    while (count--) {
        engine_sample(e, e->now + 1, mask);
        assert(engine_valid(e));
    }
}
static Engine ready(unsigned vacancy)
{
    Engine e;
    engine_init(&e);
    hold(&e, 15u ^ (1u << vacancy), 60);
    assert(e.state == READY);
    return e;
}
int main(void)
{
    /* Exhaust every vacancy/source pairing; ring adjacency is independent. */
    for (unsigned v=0; v<4; ++v) {
        for (unsigned s=0; s<4; ++s) {
            if (s==v) continue;
            unsigned base=15u^(1u<<v), lifted=base^(1u<<s);
            Engine e=ready(v);
            hold(&e,lifted,5);
            bool adjacent=((s+1)%4==v || (v+1)%4==s);
            if (!adjacent) {
                assert(e.state==HALT && e.result==NONADJACENT);
                continue;
            }
            assert(e.state==CARRY && e.starts==1 && e.source==s);
            Engine cancel=e;
            hold(&cancel,base,5);
            assert(cancel.canceled==1 && cancel.moves==0 && cancel.state==WAIT);
            hold(&e,15u^(1u<<s),5);
            assert(e.moves==1 && e.vacancy==s && e.state==WAIT);
            assert(e.clockwise==((v+1)%4==s));
            assert(e.counterclockwise==((s+1)%4==v));
            hold(&e,15u^(1u<<s),50);
            assert(e.state==READY);
        }
    }
    Engine e=ready(0);
    hold(&e,13,5); /* a direct swap has no observed lift */
    assert(e.state==HALT && e.result==AMBIGUOUS && !e.starts);
    e=ready(0); hold(&e,12,5);
    assert(e.state==CARRY);
    hold(&e,8,5);
    assert(e.state==HALT && e.abandoned==1);
    e=ready(0); hold(&e,12,5);
    hold(&e,12,DEADLINE_MS-1);
    assert(e.state==CARRY);
    hold(&e,13,1); /* deadline rejects before any arrival */
    assert(e.result==TIMEOUT && e.abandoned==1);
    e=ready(0); hold(&e,12,5);
    engine_sample(&e,e.now+2,13);
    assert(e.state==HALT && e.result==LOST && e.gaps==1);
    assert(engine_valid(&e));
    e=ready(0);
    hold(&e,12,4); hold(&e,14,6);
    assert(e.state==READY && !e.starts); /* unqualified glitch */
    e=ready(0); e.now=UINT32_MAX-2;
    hold(&e,14,5);
    assert(e.state==READY); /* modular clock wrap */
    e=ready(0); e.starts=LIMIT; e.canceled=LIMIT;
    hold(&e,12,5);
    assert(e.state==HALT && (e.fault&F_EXHAUST));
    e=ready(0); engine_fault(&e,F_CONFIG);
    assert(e.state==HALT && engine_valid(&e));
    char frame[512];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(!wire_encode(&e,frame,2));
    /* Repeated cyclic traversal, then reverse, without resetting history. */
    e=ready(0);
    for(unsigned i=0;i<40;++i) {
        unsigned s=(e.vacancy+(i<20?1:3))%4;
        unsigned mask=e.base^(1u<<s);
        hold(&e,mask,5);
        hold(&e,15u^(1u<<s),5);
        hold(&e,15u^(1u<<s),50);
    }
    assert(e.moves==40 && e.clockwise==20 && e.counterclockwise==20);
    /* Every stable arrival vector, from every legal pending lift. */
    for (unsigned v=0;v<4;++v) {
        for (unsigned s=0;s<4;++s) {
            if (!((s+1)%4==v || (v+1)%4==s)) continue;
            for (unsigned m=0;m<16;++m) {
                e=ready(v);
                unsigned transit=e.base^(1u<<s);
                hold(&e,transit,5);
                hold(&e,m,5);
                if (m==transit) assert(e.state==CARRY);
                else if (m==(15u^(1u<<v))) assert(e.canceled==1);
                else if (m==(15u^(1u<<s))) assert(e.moves==1);
                else assert(e.state==HALT && e.abandoned==1);
            }
        }
    }
    e=ready(0); hold(&e,12,5); hold(&e,13,5);
    hold(&e,9,5); /* no READY interval between two moves */
    assert(e.state==HALT && e.starts==1 && e.moves==1);
    e=ready(0); hold(&e,12,5); hold(&e,12,DEADLINE_MS-5);
    hold(&e,13,5); /* arrival qualifies exactly at the rejecting deadline */
    assert(e.result==TIMEOUT && !e.moves);
    e=ready(0); hold(&e,12,5); hold(&e,12,DEADLINE_MS-6);
    hold(&e,13,5);
    assert(e.moves==1 && e.last_ms==DEADLINE_MS-1);
    puts("PASS exhaustive ring edges, cancellations, ambiguity, gap, wrap, deadline, overflow and repeated walks");
}
