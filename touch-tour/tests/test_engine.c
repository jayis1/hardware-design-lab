/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, raw);
        if (!engine_valid(e)) {
            fprintf(stderr,"invalid now=%u samples=%u mask=%u age=%u rounds=%u abort=%u discard=%u\n",
                    e->now,e->samples,e->mask,e->age,e->rounds,e->aborted,e->discarded);
            assert(engine_valid(e));
        }
    }
}
static void tap(Engine *e, unsigned i)
{
    hold(e, 0, 6);
    hold(e, 1u << i, 6);
}
static void oracle(unsigned word, unsigned length)
{
    Engine e;
    engine_init(&e);
    unsigned mask=0, repeats=0, rounds=0, visits[4]={0};
    for (unsigned j=0; j<length; ++j) {
        unsigned i=word & 3u;
        word >>= 2;
        ++visits[i];
        if (mask & (1u<<i)) ++repeats;
        mask |= 1u<<i;
        if (mask == 15) { ++rounds; mask=0; }
        tap(&e, i);
        assert(e.mask==mask && e.repeats==repeats && e.rounds==rounds);
        for (unsigned k=0; k<4; ++k) assert(e.visits[k]==visits[k]);
    }
}
int main(void)
{
    /* Every eight-visit word: includes repeats, arbitrary order and rollover. */
    for (unsigned word=0; word<65536; ++word) oracle(word,8);
    Engine e; engine_init(&e);
    hold(&e,1,20); assert(e.starts==0); /* held boot does not count */
    tap(&e,0); tap(&e,1); tap(&e,2); tap(&e,3);
    assert(e.rounds==1 && e.last_order==228 && e.last_age==36);
    hold(&e,8,20); assert(e.starts==1); /* held completion cannot restart */
    tap(&e,0); hold(&e,3,1);
    assert(e.aborted==1 && e.discarded==1 && !e.mask);
    hold(&e,1,20); assert(e.starts==2);
    tap(&e,2); assert(e.starts==3);
    engine_sample(&e,e.now+10,0);
    assert(e.aborted==2 && e.missed==9 && engine_valid(&e));
    tap(&e,0); hold(&e,0,6);
    hold(&e,0,DEADLINE-12);
    hold(&e,2,6); /* qualifying visit exactly at rejecting timeout */
    assert(e.timeouts==1 && !e.active && e.visits[1]==1);
    engine_init(&e); hold(&e,0,6); hold(&e,1,1); hold(&e,2,6);
    assert(e.starts==0 && e.disruptions==1); /* sequential handoff */
    engine_init(&e); tap(&e,0); e.samples=UINT32_MAX;
    engine_sample(&e,e.now+1,0);
    assert(e.fault==F_OVERFLOW && e.aborted==1 && !e.active);
    engine_init(&e); hold(&e,0,6); e.now=UINT32_MAX;
    engine_sample(&e,0,0); assert(!e.fault);
    char frame[1024];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(wire_encode(&e,frame,3)==0);
    assert(wire_encode(&e,NULL,0)==0);
    /* Every raw-to-raw pair including startup-held and overlapping pads. */
    for (unsigned a=0; a<16; ++a) {
        for (unsigned b=0; b<16; ++b) {
            engine_init(&e); hold(&e,0,6);
            hold(&e,a,6); hold(&e,b,6); hold(&e,0,6);
        }
    }
    /* Compare acceptance one sample before versus exactly at the deadline. */
    for (unsigned offset=0; offset<2; ++offset) {
        engine_init(&e); tap(&e,0); tap(&e,1); tap(&e,2);
        hold(&e,0,6); hold(&e,0,DEADLINE-37+offset);
        hold(&e,8,6);
        assert(e.rounds==1-offset && e.timeouts==offset);
    }
    engine_init(&e); tap(&e,0);
    engine_sample(&e,e.now,0);
    assert(e.fault==F_TIME && e.aborted==1 && engine_valid(&e));
    uint32_t frozen=e.samples;
    engine_sample(&e,e.now+1,0);
    assert(e.samples==frozen && engine_valid(&e));
    puts("PASS: 65536 eight-visit oracle words, boundaries, gaps, overflow, framing");
}
