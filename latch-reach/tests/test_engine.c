/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned ticks)
{
    while (ticks--) {
        engine_sample(e, e->now + 1, (uint8_t)raw);
        assert(engine_valid(e));
    }
}
static void ready(Engine *e)
{
    engine_init(e);
    hold(e, 1, 110);
    assert(e->phase == READY);
}
static void trip(Engine *e, unsigned seen, unsigned latch)
{
    hold(e, 0, 10);
    if (seen & 1) hold(e, 2, 10);
    hold(e, 0, 10);
    if (seen & 2) hold(e, 4, 10);
    hold(e, 0, 10);
    hold(e, 1 | latch, 110);
}
int main(void)
{
    Engine e;
    ready(&e);
    hold(&e, 2, 5); /* HOME falls and NEAR qualifies on the same sample */
    hold(&e, 0, 10);
    hold(&e, 1, 110);
    assert(e.bins[1][0]==1);
    for (unsigned seen=0; seen<4; ++seen) {
        for (unsigned latch=0; latch<2; ++latch) {
            ready(&e);
            trip(&e, seen, latch*8);
            assert(e.starts==1 && e.completed==1 && !e.rejected);
            assert(e.bins[seen][latch]==1);
        }
    }
    ready(&e);
    hold(&e, 0, 10);
    hold(&e, 2, 10);
    hold(&e, 1, 20);
    hold(&e, 9, 1); /* raw-only latch excursion is disturbed evidence */
    hold(&e, 1, 110);
    assert(e.bins[1][2]==1);
    ready(&e);
    hold(&e, 0, 10);
    hold(&e, 3, 1); /* impossible raw spatial overlap rejects immediately */
    assert(e.rejected==1 && !e.completed);
    hold(&e, 1, 110);
    assert(e.phase==READY);
    ready(&e);
    hold(&e, 0, 10);
    engine_sample(&e,e.now+2,0);
    assert(e.fault==F_GAP && e.rejected==1 && e.phase==HALT);
    uint32_t seq=e.sequence;
    hold(&e,9,200);
    assert(e.sequence==seq && !e.completed);
    ready(&e);
    hold(&e,0,10);
    hold(&e,1,50);
    hold(&e,0,1); /* departure during return window is rejection */
    assert(e.rejected==1);
    ready(&e);
    hold(&e,0,10);
    while (e.age < DEADLINE-1) hold(&e,0,1);
    hold(&e,1,1);
    assert(e.rejected==1 && !e.completed);
    ready(&e);
    e.sequence=LIMIT;
    hold(&e,0,1);
    assert(e.fault==F_OVERFLOW && e.phase==HALT);
    ready(&e);
    e.now=0xfffffffeu;
    hold(&e,1,3);
    assert(!e.fault && e.now==1);
    char buffer[768];
    /* All six-symbol words over clear/NEAR/FAR, independent set oracle. */
    for (unsigned word=0; word<729; ++word) {
        ready(&e);
        hold(&e,0,10);
        unsigned code=word, oracle=0;
        for (unsigned index=0; index<6; ++index) {
            unsigned symbol=code%3;
            code/=3;
            unsigned raw=symbol==0 ? 0 : (symbol==1 ? 2 : 4);
            oracle |= symbol;
            hold(&e,raw,10);
        }
        hold(&e,1,110);
        assert(e.bins[oracle][0]==1 && e.completed==1);
    }
    /* One sample before deadline succeeds; exactly at deadline rejects. */
    for (unsigned tied=0; tied<2; ++tied) {
        ready(&e); hold(&e,0,10);
        while (e.age<DEADLINE-106+tied) hold(&e,0,1);
        hold(&e,1,105);
        assert(e.completed==1-tied && e.rejected==tied);
    }
    ready(&e); hold(&e,0,10);
    e.sequence=LIMIT;
    hold(&e,1,1);
    assert(e.phase==HALT && e.starts==1 && e.rejected==1 && !e.completed);
    assert(wire_encode(&e,buffer,sizeof buffer)>0);
    assert(!wire_encode(&e,buffer,5));
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS: reach matrix, raw disturbances, rearm, deadline, gap, wrap, overflow, wire");
}
