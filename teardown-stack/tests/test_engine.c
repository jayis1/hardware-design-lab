/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    while (ticks--) engine_sample(e, e->now + 1u, mask);
}
static void ready(Engine *e)
{
    engine_init(e);
    hold(e, 0, 30);
    assert(e->state == READY);
}
static void edge(Engine *e, unsigned mask)
{
    hold(e, mask, 12);
}
static void retrievals(const unsigned *deposit, unsigned *take, unsigned n)
{
    if (n == 4) {
        Engine e;
        ready(&e);
        unsigned mask = 0;
        bool reverse = true;
        for (unsigned i = 0; i < 4; ++i) {
            mask |= 1u << deposit[i];
            edge(&e, mask);
            reverse &= take[i] == deposit[3u-i];
        }
        for (unsigned i = 0; i < 4; ++i) {
            mask &= ~(1u << take[i]);
            edge(&e, mask);
            assert(engine_valid(&e));
        }
        assert(e.completed == (unsigned)reverse);
        assert(e.aborted == (unsigned)!reverse);
        assert(e.order_errors == (unsigned)!reverse);
        assert(e.state == READY);
        return;
    }
    for (unsigned i = n; i < 4; ++i) {
        unsigned t = take[i]; take[i] = take[n]; take[n] = t;
        retrievals(deposit, take, n+1);
        t = take[i]; take[i] = take[n]; take[n] = t;
    }
}
static void permutations(unsigned *p, unsigned n)
{
    if (n == 4) {
        unsigned take[] = {0, 1, 2, 3};
        retrievals(p, take, 0);
        Engine e;
        ready(&e);
        unsigned mask = 0;
        for (unsigned i=0; i<4; ++i) {
            mask |= 1u << p[i]; edge(&e, mask);
            assert(e.depth == i+1);
            assert(e.stack[i] == p[i]);
        }
        for (unsigned i=4; i>0; --i) {
            mask &= ~(1u << p[i-1]); edge(&e, mask);
        }
        assert(e.completed == 1 && e.pushes == 4 && e.pops == 4);
        assert(e.aborted == 0 && e.state == READY);
        return;
    }
    for (unsigned i=n; i<4; ++i) {
        unsigned t=p[i]; p[i]=p[n]; p[n]=t;
        permutations(p,n+1);
        t=p[i]; p[i]=p[n]; p[n]=t;
    }
}
int main(int argc, char **argv)
{
    Engine e;
    unsigned p[]={0,1,2,3};
    permutations(p,0);
    ready(&e);
    uint32_t random = 1234567u;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        engine_sample(&e, e.now + ((i % 113u) ? 1u : 3u), (random >> 24) & 15u);
        assert(engine_valid(&e));
    }
    ready(&e);
    /* Chatter on B must not postpone a sustained A closure. */
    for (unsigned i = 0; i < 12; ++i)
        engine_sample(&e, e.now + 1u, 1u | ((i & 1u) << 1));
    assert(e.depth == 1 && e.stack[0] == 0);
    ready(&e); edge(&e,1); edge(&e,3); edge(&e,2);
    assert(e.order_errors==1 && e.aborted==1 && e.abandoned==2);
    assert(e.state==WAIT && e.depth==0);
    edge(&e,0); assert(e.state==READY);
    edge(&e,4); edge(&e,0); assert(e.completed==1);
    ready(&e); edge(&e,3);
    assert(e.collisions==1 && e.started==0 && e.state==WAIT);
    edge(&e,0); edge(&e,1); edge(&e,2);
    assert(e.collisions==2 && e.aborted==1);
    ready(&e); edge(&e,1); edge(&e,3); edge(&e,1); edge(&e,5);
    assert(e.pushes==3 && e.pops==1 && e.depth==2);
    edge(&e,1); edge(&e,0); assert(e.completed==1);
    ready(&e); edge(&e,1);
    engine_sample(&e,e.now+3,1);
    assert(e.gaps==1 && e.aborted==1 && e.state==WAIT);
    edge(&e,1); assert(e.state==WAIT);
    edge(&e,0); assert(e.state==READY);
    ready(&e); hold(&e,1,3); hold(&e,0,12);
    assert(e.pushes==0);
    engine_init(&e); hold(&e,1,30);
    assert(e.started==0 && e.state==WAIT);
    edge(&e,0); edge(&e,1); assert(e.started==1);
    ready(&e); e.now=UINT32_MAX-5; edge(&e,1);
    assert(e.pushes==1 && e.gaps==0);
    ready(&e); edge(&e,1);
    e.pushes=UINT32_MAX; e.pops=UINT32_MAX-1;
    edge(&e,3);
    assert(e.state==HALT && e.fault==F_OVERFLOW);
    assert(e.pushes==e.pops+e.abandoned && e.depth==0);
    ready(&e); edge(&e,1); engine_fault(&e,F_CONFIG);
    assert(e.aborted==1 && e.abandoned==1 && e.state==HALT);
    char frame[768];
    assert(wire_encode(&e,frame,sizeof frame));
    assert(!wire_encode(&e,frame,2));
    e.depth=1; assert(!wire_encode(&e,frame,sizeof frame));
    if (argc>1 && !strcmp(argv[1],"--fixture")) {
        ready(&e);
        wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        edge(&e,2); wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        edge(&e,10); wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        edge(&e,2); wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
        edge(&e,0); wire_encode(&e,frame,sizeof frame); fputs(frame,stdout);
    } else puts("PASS: 576 deposit/retrieval permutations, 100000 samples and edge/fault/conservation cases");
    return 0;
}
