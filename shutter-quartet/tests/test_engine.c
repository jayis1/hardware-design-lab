/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, e->time + 1u, (uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void ready(Engine *e)
{
    engine_init(e);
    hold(e, 0, 300);
    assert(e->state == READY);
}
static void pulse(Engine *e, unsigned mask, unsigned width)
{
    hold(e, mask, width);
    hold(e, 0, 20);
}
int main(int argc, char **argv)
{
    Engine e;
    ready(&e);
    pulse(&e, 15, 40);
    hold(&e, 0, 300);
    assert(e.completed == 1 && e.starts == 1 && e.seen == 15);
    for (unsigned i=0; i<4; ++i) {
        assert(e.lit[i] == 40 && e.pulses[i] == 1);
        assert(e.first[i] == 0 && e.last[i] == 40);
    }
    assert(e.span == 40);
    if (argc > 1 && !strcmp(argv[1], "--fixture")) {
        char frame[768];
        for (unsigned i=0; i<4; ++i) {
            hold(&e,0,100);
            assert(wire_encode(&e,frame,sizeof frame));
            fputs(frame,stdout);
        }
        return 0;
    }
    /* Dark gaps inside the capture preserve fragmented exposure. */
    ready(&e);
    pulse(&e, 1, 30);
    pulse(&e, 2, 60);
    pulse(&e, 1, 20);
    hold(&e,0,300);
    assert(e.seen==3 && e.lit[0]==50 && e.lit[1]==60);
    assert(e.pulses[0]==2 && e.pulses[1]==1);
    assert(e.first[1]>e.first[0] && e.last[0]>e.last[1]);
    assert(e.lit[2]==0 && e.pulses[2]==0);
    /* Independent qualification: chatter B cannot postpone stable A. */
    ready(&e);
    for (unsigned i=0;i<30;++i) hold(&e,1|((i&1)?2:0),1);
    hold(&e,0,300);
    assert(e.seen==1 && e.lit[0]==30);
    /* Lit at attachment is not a witnessed opening. */
    engine_init(&e);
    hold(&e,15,300);
    assert(e.starts==0 && e.state==WAIT);
    hold(&e,0,300);
    assert(e.state==READY);
    /* A raw interruption restarts baseline qualification. */
    ready(&e);
    pulse(&e,1,20);
    hold(&e,0,200);
    assert(e.state==ACTIVE);
    hold(&e,2,1);
    hold(&e,0,250);
    assert(e.state==ACTIVE);
    hold(&e,0,1);
    assert(e.state==READY && e.completed==1);
    /* Gap aborts current evidence; completed evidence survives. */
    pulse(&e,4,20);
    engine_sample(&e,e.time+5,0);
    assert(engine_valid(&e) && e.aborted==1 && e.gaps==1);
    assert(e.completed==1 && e.seen==1);
    hold(&e,0,300);
    assert(e.state==READY);
    /* Deadline precedes a simultaneous closing edge. */
    ready(&e);
    hold(&e,1,6);
    assert(e.state==ACTIVE && e.age==0);
    hold(&e,1,LIMIT_MS-QUALIFY_MS);
    hold(&e,0,QUALIFY_MS);
    assert(e.state==WAIT && e.timeouts==1 && e.aborted==1);
    hold(&e,0,300);
    assert(e.state==READY);
    /* One-tick modular wrap is continuous. */
    ready(&e);
    e.time=UINT32_MAX;
    hold(&e,0,1);
    assert(e.time==0 && e.gaps==0);
    /* Counter exhaustion leaves conserved terminal evidence. */
    ready(&e);
    e.starts=UINT32_MAX; e.aborted=UINT32_MAX;
    hold(&e,1,6);
    assert(e.state==HALTED && (e.flags & F_OVERFLOW));
    ready(&e);
    hold(&e,1,6);
    e.gaps=UINT32_MAX; e.flags=F_GAP;
    engine_sample(&e,e.time+2,1);
    assert(engine_valid(&e) && e.state==HALTED && e.aborted==1);
    /* Encoder rejects contradictory completed evidence and small buffers. */
    ready(&e); pulse(&e,15,40); hold(&e,0,300);
    char frame[768];
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,5)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    e.lit[0]=41;
    assert(!engine_valid(&e) && wire_encode(&e,frame,sizeof frame)==0);
    /* All per-channel pulse widths, in staggered independent windows. */
    for (unsigned mask=1;mask<16;++mask) {
        ready(&e); pulse(&e,mask,17+mask); hold(&e,0,300);
        assert(e.seen==mask && e.completed==1);
        for (unsigned i=0;i<4;++i)
            assert(e.lit[i]==((mask&(1u<<i))?17+mask:0));
    }
    /* Independent time-window oracle: every opening order crossed with
     * every closing order. Crossing fronts and ties are not rejected. */
    unsigned trials=0;
    for (unsigned a=0;a<4;++a) for (unsigned b=0;b<4;++b)
    for (unsigned c=0;c<4;++c) for (unsigned d=0;d<4;++d) {
        if (a==b || a==c || a==d || b==c || b==d || c==d) continue;
        unsigned opening_order[4]={a,b,c,d};
        for (unsigned j=0;j<4;++j) for (unsigned k=0;k<4;++k)
        for (unsigned l=0;l<4;++l) for (unsigned m=0;m<4;++m) {
            if (j==k || j==l || j==m || k==l || k==m || l==m) continue;
            unsigned closing_order[4]={j,k,l,m};
            unsigned on[4],off[4];
            for (unsigned i=0;i<4;++i) {
                on[opening_order[i]]=10+10*i;
                off[closing_order[i]]=80+10*i;
            }
            ready(&e);
            for (unsigned t=0;t<400;++t) {
                unsigned raw=0;
                for (unsigned i=0;i<4;++i)
                    if (t>=on[i] && t<off[i]) raw|=1u<<i;
                hold(&e,raw,1);
            }
            assert(e.completed==1 && e.seen==15);
            for (unsigned i=0;i<4;++i) {
                assert(e.first[i]==on[i]-10);
                assert(e.last[i]==off[i]-10);
                assert(e.lit[i]==off[i]-on[i]);
            }
            ++trials;
        }
    }
    assert(trials==576);
    puts("PASS shutter engine, 576 opening/closing orders, faults, boundaries and encoder");
    return 0;
}
