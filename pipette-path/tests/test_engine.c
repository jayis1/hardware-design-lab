/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static bool fixture;
static void hold(Engine *e, unsigned mask, unsigned ms)
{
    while (ms--) {
        engine_sample(e, e->now + 1u, mask);
        assert(engine_valid(e));
    }
    if (fixture) {
        char record[512];
        assert(wire_encode(e,record,sizeof record));
        fputs(record,stdout);
    }
}
static void ready(Engine *e)
{
    engine_init(e);
    hold(e, 1, 10);
    assert(e->phase == READY);
}
static void aspirate(Engine *e)
{
    hold(e, 0, 10);
    hold(e, 2, 10);
    assert(e->phase == FIRST);
    hold(e, 10, 10);
    assert(e->phase == IMMERSED);
    hold(e, 8, 10);
    hold(e, 9, 10);
    assert(e->phase == ASPIRATED);
}
static void complete(Engine *e)
{
    hold(e, 1, 10);
    hold(e, 0, 10);
    hold(e, 2, 10);
    assert(e->phase == DISPENSE);
    hold(e, 6, 10);
    assert(e->phase == BLOWOUT);
    hold(e, 2, 10);
    hold(e, 0, 10);
    hold(e, 1, 10);
}
int main(int argc, char **argv)
{
    (void)argv;
    Engine e;
    char frame[512];
    fixture=argc>1;
    ready(&e);
    aspirate(&e); complete(&e);
    assert(e.starts == 1 && e.completed == 1 && e.aborted == 0);
    assert(e.last_reason == 0 && e.last_ms > 0);
    if (argc > 1) return 0;
    ready(&e); aspirate(&e); hold(&e,1,10); hold(&e,9,10);
    assert(e.aborted==1 && e.last_reason==E_SOURCE);
    ready(&e); aspirate(&e); hold(&e,0,10);
    assert(e.aborted==1 && e.last_reason==E_SOURCE);
    ready(&e); e.phase=DISPENSE; e.levels=15; e.starts=1;
    assert(!engine_valid(&e));
    ready(&e); aspirate(&e); complete(&e);
    /* Repeated use, source release, and missing second stop. */
    aspirate(&e); hold(&e,1,10); hold(&e,2,10); hold(&e,1,10);
    assert(e.aborted == 1 && e.last_reason == E_ORDER);
    ready(&e); hold(&e,2,10); hold(&e,10,10); hold(&e,0,10);
    assert(e.aborted == 1 && e.last_reason == E_SOURCE);
    ready(&e); hold(&e,6,10);
    assert(e.aborted == 1 && e.last_reason == E_ORDER);
    ready(&e); hold(&e,9,10); hold(&e,10,10);
    assert(e.starts == 0 && e.phase == WAIT);
    /* Startup mid-stroke cannot authorize a cycle. */
    engine_init(&e); hold(&e,2,10); hold(&e,10,10); hold(&e,9,10);
    assert(e.starts == 0);
    hold(&e,1,10); assert(e.phase == READY);
    /* Contradictory plunger codes abort, including home+first. */
    ready(&e); hold(&e,0,10); hold(&e,3,10);
    assert(e.last_reason == E_CODE && e.aborted == 1);
    /* A missed sample breaks correlation and needs a new baseline. */
    ready(&e); hold(&e,2,10);
    engine_sample(&e,e.now+2,2);
    assert(engine_valid(&e) && e.aborted == 1 && e.gaps == 1);
    hold(&e,2,10); assert(e.phase == WAIT);
    hold(&e,1,10); assert(e.phase == READY);
    /* Timeout is inclusive and dominates arrival on the deadline. */
    ready(&e); hold(&e,2,10);
    e.elapsed=TIMEOUT_MS-1;
    engine_sample(&e,e.now+1,2);
    assert(e.phase == WAIT && e.last_reason == E_TIMEOUT);
    /* Timer wrap is not a gap. */
    ready(&e); e.now=UINT32_MAX-2; hold(&e,1,5);
    assert(!e.gaps && e.phase == READY);
    /* Independent qualifiers do not postpone first-stop qualification. */
    ready(&e); hold(&e,0,10);
    for (unsigned i=0;i<5;i++) engine_sample(&e,e.now+1,2|((i&1)?8:0));
    assert(e.phase == FIRST);
    /* Subthreshold pulse rejected. */
    ready(&e); hold(&e,6,4); hold(&e,1,10); assert(e.starts==0);
    /* Transactional overflow must preserve accounting. */
    ready(&e); e.starts=UINT32_MAX; e.completed=UINT32_MAX;
    hold(&e,0,10);
    assert(e.faults == F_OVERFLOW && e.starts == UINT32_MAX);
    assert(e.completed == UINT32_MAX && e.aborted == 0);
    ready(&e); hold(&e,2,10); e.tx_errors=UINT32_MAX;
    engine_tx_error(&e);
    assert(engine_valid(&e) && e.aborted==1 && e.faults==F_OVERFLOW);
    /* Exhaustive mask pairs from reset; every output must serialize. */
    for (unsigned a=0;a<16;a++) for(unsigned b=0;b<16;b++) {
        ready(&e); hold(&e,a,10); hold(&e,b,10);
        assert(wire_encode(&e,frame,sizeof frame));
    }
    /* Exercise arbitrary inputs from every reachable instructional phase. */
    ready(&e);
    const unsigned path[]={0,2,10,8,9,1,0,2,6,2,0,1};
    for (unsigned stage=0;stage<sizeof path/sizeof path[0];stage++) {
        hold(&e,path[stage],10);
        for (unsigned a=0;a<16;a++) for (unsigned b=0;b<16;b++) {
            Engine trial=e;
            hold(&trial,a,10); hold(&trial,b,10);
            assert(wire_encode(&trial,frame,sizeof frame));
        }
    }
    ready(&e);
    assert(wire_crc("123456789",9)==0x29b1);
    assert(!wire_encode(&e,frame,8));
    e.starts=1; assert(!wire_encode(&e,frame,sizeof frame));
    puts("PASS sequence, errors, gap, wrap, qualification, overflow, 256 mask pairs, CRC");
}
