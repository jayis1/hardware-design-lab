/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void ticks(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1u, (uint8_t)mask);
        assert(engine_valid(e));
    }
}
static unsigned code(unsigned stage)
{
    static const unsigned values[] = {0, 1, 3};
    return values[stage];
}
static void matrix(void)
{
    /* Independent oracle: every ordered pair of local stage positions.
     * Stable intervals after five observations occupy exactly one cell. */
    for (unsigned a=0; a<3; ++a) {
        for (unsigned b=0; b<3; ++b) {
            Engine e; engine_init(&e);
            unsigned mask=code(a) | (code(b)<<2);
            ticks(&e,mask,5);
            assert(e.unknown==4 && e.intervals==4);
            ticks(&e,mask,17);
            for (unsigned cell=0; cell<9; ++cell)
                assert(e.bins[cell]==(cell==3*a+b ? 17u : 0u));
            assert(e.episodes==(a && b ? 1u : 0u));
            assert(e.run==(a && b ? 17u : 0u));
        }
    }
}
static void transitions(void)
{
    for (unsigned x=0; x<9; ++x) {
        for (unsigned y=0; y<9; ++y) {
            Engine e; engine_init(&e);
            unsigned from=code(x/3)|(code(x%3)<<2);
            unsigned to=code(y/3)|(code(y%3)<<2);
            ticks(&e,from,10);
            uint32_t before=e.bins[y];
            ticks(&e,to,10);
            assert(e.bins[y]-before==(x==y ? 10u : 5u));
            assert(e.unknown==(x==y ? 4u : 9u));
        }
    }
}
int main(void)
{
    matrix(); transitions();
    Engine e; engine_init(&e);
    ticks(&e,5,15);
    assert(e.run==10 && e.maximum==10 && e.episodes==1);
    ticks(&e,4,1); /* Rejected glitch splits exposure, not an invisible bridge. */
    assert(!e.run);
    ticks(&e,5,1); assert(!e.run);
    ticks(&e,5,1); assert(e.run==1 && e.episodes==2);
    ticks(&e,7,5); /* Stage changes also split matrix intervals. */
    assert(!e.run);
    ticks(&e,7,1); assert(e.run==1 && e.episodes==3);
    Engine bad; engine_init(&bad);
    ticks(&bad,2,20); /* Forbidden thermometer code is unknown, not stage two. */
    assert(bad.unknown==19 && bad.invalid==16 && !bad.fault);
    engine_sample(&bad,bad.now+2,0);
    assert(bad.fault==F_GAP && bad.phase==HALT && bad.gaps==1);
    Engine frozen=bad;
    ticks(&bad,15,20);
    assert(bad.sequence==frozen.sequence && bad.intervals==frozen.intervals);
    engine_init(&bad); bad.now=UINT32_MAX-3;
    ticks(&bad,0,10); assert(!bad.fault);
    engine_init(&bad); ticks(&bad,0,10);
    bad.sequence=LIMIT; bad.intervals=LIMIT-1; bad.unknown=LIMIT-1;
    memset(bad.bins,0,sizeof bad.bins);
    assert(engine_valid(&bad));
    ticks(&bad,0,1); assert(bad.fault==F_OVERFLOW && bad.sequence==LIMIT);
    engine_init(&bad); engine_sample(&bad,1,16);
    assert(bad.fault==F_CONFIG && engine_valid(&bad));
    engine_init(&bad);
    bad.sequence=2; bad.intervals=1; bad.bins[0]=1;
    assert(!engine_valid(&bad));
    /* Final permitted shared interval, then freeze on the next sample. */
    engine_init(&bad); ticks(&bad,5,10);
    bad.sequence=LIMIT-1; bad.intervals=LIMIT-2;
    bad.bins[4]=LIMIT-6; bad.run=LIMIT-6; bad.maximum=LIMIT-6;
    assert(engine_valid(&bad));
    ticks(&bad,5,1);
    assert(bad.sequence==LIMIT && bad.bins[4]==LIMIT-5);
    ticks(&bad,5,1); assert(bad.fault==F_OVERFLOW && bad.run==0);
    engine_init(&bad); ticks(&bad,2,10);
    bad.sequence=LIMIT-1; bad.intervals=LIMIT-2;
    bad.unknown=LIMIT-2; bad.invalid=LIMIT-5;
    assert(engine_valid(&bad)); ticks(&bad,2,1);
    assert(bad.invalid==LIMIT-4 && bad.unknown==LIMIT-1);
    bad.tx_errors=LIMIT-1; engine_tx_error(&bad);
    assert(bad.tx_errors==LIMIT && !bad.fault);
    engine_tx_error(&bad); assert(bad.fault==F_OVERFLOW && engine_valid(&bad));
    char frame[768];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,frame,sizeof frame));
    assert(!strncmp(frame,"AY1,",4));
    assert(!wire_encode(&e,frame,8));
    ++e.bins[0]; assert(!engine_valid(&e));
    assert(!wire_encode(&e,frame,sizeof frame));
    puts("PASS all 9 stages, 81 transitions, glitch splits, invalid codes, gap freeze, wrap, overflow and wire bounds");
}
