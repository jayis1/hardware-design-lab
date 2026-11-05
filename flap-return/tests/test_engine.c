/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void step(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1u, (uint8_t)mask);
        assert(engine_valid(e));
    }
}
static Engine released(void)
{
    Engine e;
    engine_init(&e);
    step(&e, 8, 260);
    assert(e.phase == READY);
    step(&e, 0, 5);
    assert(e.phase == ACTIVE && e.age == 0);
    return e;
}
int main(void)
{
    Engine e = released();
    step(&e, 1, 5);
    assert(e.lane[0].first == 5 && e.lane[0].closures == 1);
    step(&e, 0, 5);
    assert(e.lane[0].reopens == 1);
    step(&e, 7, 5);
    assert(e.lane[0].last == 15 && e.lane[0].closures == 2);
    step(&e, 7, 249);
    assert(e.phase == ACTIVE);
    step(&e, 7, 1);
    assert(e.phase == DONE && e.age == 265);
    Lane saved[3]; memcpy(saved,e.lane,sizeof saved);
    step(&e, 8, 1000);
    assert(e.age == 265 && !memcmp(saved,e.lane,sizeof saved));
    char frame[512];
    assert(wire_encode(&e,frame,sizeof frame)>0);
    assert(!wire_encode(&e,frame,8));
    e = released(); step(&e,8,1);
    assert(e.phase==HALT && e.fault==F_REHELD);
    e = released(); engine_sample(&e,e.now+2,7);
    assert(e.phase==HALT && e.fault==F_GAP);
    e = released(); step(&e,0,DEADLINE_MS);
    assert(e.phase==HALT && e.fault==F_TIMEOUT && e.age==DEADLINE_MS);
    e = released(); step(&e,7,5); step(&e,7,249);
    step(&e,6,1); step(&e,7,1);
    assert(e.phase==ACTIVE && e.lane[0].quiet==0);
    step(&e,7,250); assert(e.phase==DONE);
    engine_init(&e); step(&e,0,500); assert(e.phase==WAIT);
    step(&e,8,260); step(&e,9,1);
    assert(e.phase==HALT && e.fault==F_PRECLOSED);
    /* Enumerate all short closure/open words against independent histories.
     * Every symbol lasts five samples, so each changed bit qualifies once. */
    for (unsigned word=0; word<4096; ++word) {
        e=released(); unsigned code=word, old=0;
        unsigned close[3]={0}, reopen[3]={0}, first[3]={0}, last[3]={0};
        for (unsigned pos=0;pos<4;++pos) {
            unsigned next=code&7u; code>>=3;
            for (unsigned i=0;i<3;++i) {
                unsigned bit=1u<<i;
                if ((next&bit) && !(old&bit)) {
                    if (!close[i]) first[i]=5*(pos+1);
                    ++close[i]; last[i]=5*(pos+1);
                }
                if (!(next&bit) && (old&bit)) ++reopen[i];
            }
            step(&e,next,5); old=next;
            for (unsigned i=0;i<3;++i) {
                assert(e.lane[i].closures==close[i]);
                assert(e.lane[i].reopens==reopen[i]);
                assert(e.lane[i].first==first[i]);
                assert(e.lane[i].last==last[i]);
            }
        }
    }
    /* A newly closing peer does not postpone an independent qualification. */
    e=released(); step(&e,1,3); step(&e,3,2);
    assert(e.lane[0].first==5 && e.lane[1].closures==0);
    step(&e,3,3); assert(e.lane[1].first==8);
    /* Deadline wins over completion on the same sample. */
    e=released(); step(&e,0,DEADLINE_MS-255);
    step(&e,7,255); assert(e.phase==HALT && e.fault==F_TIMEOUT);
    e=released(); step(&e,0,DEADLINE_MS-256);
    step(&e,7,255); assert(e.phase==DONE && e.age==DEADLINE_MS-1);
    /* Frozen halt retains measurements even while physical contacts move. */
    e=released(); step(&e,1,5); step(&e,9,1);
    memcpy(saved,e.lane,sizeof saved);
    uint32_t halted_age=e.age;
    step(&e,7,500);
    assert(e.phase==HALT && e.age==halted_age);
    assert(!memcmp(saved,e.lane,sizeof saved));
    /* Continuous randomized contact histories exercise serializer invariants. */
    uint32_t random=12345;
    for (unsigned trial=0;trial<100;++trial) {
        e=released();
        for (unsigned t=0;t<2000;++t) {
            random=random*1664525u+1013904223u;
            step(&e,(random>>28)&7u,1);
            assert(wire_encode(&e,frame,sizeof frame));
        }
    }
    /* Tick wrap is ordinary, not a reset. */
    engine_init(&e); engine_sample(&e,UINT32_MAX-100,8);
    step(&e,8,260); assert(e.phase==READY);
    puts("PASS engine: 4096 independent words and boundary regressions");
}
