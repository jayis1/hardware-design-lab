/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>
static uint32_t tick;
static void step(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, ++tick, raw);
        assert(engine_valid(e));
    }
}
static void trial(Engine *e, unsigned pair, unsigned result)
{
    step(e, 0, 8);
    step(e, pair | (result << 2), 510);
    assert(e->state == DONE);
}
/* Independent finite assignment oracle: half-deadband units, translation
 * invariant. Three variables need no span greater than six such units. */
static bool feasible(unsigned evidence)
{
    for (int a=0;a<=6;++a) for (int b=0;b<=6;++b) for (int c=0;c<=6;++c) {
        int d[3]={a-b,a-c,b-c};
        bool ok=true;
        for (unsigned pair=0;pair<3;++pair) {
            if ((evidence & (1u<<(3*pair))) && (d[pair]<-2 || d[pair]>2)) ok=false;
            if ((evidence & (2u<<(3*pair))) && d[pair]<=2) ok=false;
            if ((evidence & (4u<<(3*pair))) && d[pair]>=-2) ok=false;
        }
        if (ok) return true;
    }
    return false;
}
int main(void)
{
    for (unsigned mask=0;mask<512;++mask)
        assert(balance_conflict(mask)==!feasible(mask));
    /* A~B, B~C, A>C is feasible: neutral is NOT equality. */
    assert(!balance_conflict(1u | (2u<<3) | (1u<<6)));
    Engine e;
    engine_init(&e); tick=0;
    step(&e,1,600); assert(e.starts==0); /* no parked baseline */
    trial(&e,1,1); assert(e.evidence==2 && e.accepted==1);
    step(&e,1|4,600); assert(e.accepted==1); /* no automatic repeat */
    trial(&e,1,2); assert(e.conflict && e.accepted==2);
    trial(&e,2,0); assert(e.accepted==3);
    char out[512]; assert(wire_encode(&e,out,sizeof out));
    assert(!wire_encode(&e,out,8));
    engine_init(&e); step(&e,0,8); step(&e,5,5);
    assert(e.state==ACTIVE && e.dwell==0);
    step(&e,5,499); assert(e.accepted==0);
    step(&e,5,1); assert(e.accepted==1);
    engine_init(&e); step(&e,0,8); step(&e,5,100);
    step(&e,1,1); assert(e.dwell==0); /* raw glitch resets settlement */
    step(&e,5,500); assert(!e.accepted);
    step(&e,5,1); assert(e.accepted==1);
    engine_init(&e); step(&e,0,8); step(&e,5,10);
    step(&e,6,1); assert(e.rejected==1 && e.state==DONE);
    step(&e,6,600); assert(!e.accepted);
    engine_init(&e); step(&e,0,8); step(&e,13,505);
    assert(e.rejected==1 && !e.accepted); /* both beam contacts */
    engine_init(&e); step(&e,0,8); step(&e,5,5);
    e.age=9999; e.dwell=499; step(&e,5,1);
    assert(e.rejected==1 && e.accepted==0); /* exact timeout wins */
    engine_init(&e); step(&e,0,8); step(&e,5,5);
    engine_sample(&e,tick+2,5); assert(e.state==HALT && e.aborted==1 && engine_valid(&e));
    engine_init(&e); step(&e,0,8); e.starts=UINT32_MAX; e.rejected=UINT32_MAX;
    step(&e,5,5); assert(e.state==HALT && engine_valid(&e));
    engine_init(&e); tick=UINT32_MAX-10; trial(&e,3,2); assert(e.accepted==1);
    puts("PASS 512 deadband graphs, neutral nontransitivity, settle boundaries, raw glitches, pairing, timeout, gap, overflow and wrap");
}
