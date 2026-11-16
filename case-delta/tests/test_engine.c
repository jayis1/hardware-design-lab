/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void hold(Engine *e, unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1u, (uint8_t)raw);
        if (!engine_valid(e)) {
            fprintf(stderr,"invalid seq=%u phase=%u fault=%u done=%u age=%u last_age=%u\n",
                    e->sequence,e->phase,e->fault,e->completed,e->age,e->last_age);
            assert(engine_valid(e));
        }
    }
}
static Engine baseline(unsigned pockets)
{
    Engine e;
    engine_init(&e);
    hold(&e, pockets | 8u, 105);
    assert(e.phase == READY && e.reference == pockets);
    return e;
}
int main(void)
{
    /* Exhaust every closed baseline, intermediate inventory and final inventory.
     * Independent set algebra is the oracle, not the engine transition code. */
    for (unsigned before=0; before<8; ++before)
        for (unsigned middle=0; middle<8; ++middle)
            for (unsigned after=0; after<8; ++after) {
                Engine e=baseline(before);
                hold(&e, before, 5);
                hold(&e, middle, 5);
                hold(&e, after, 5);
                hold(&e, after|8u, 105);
                assert(e.completed==1 && e.aborted==0 && e.starts==1);
                assert(e.last_before==before && e.last_after==after);
                assert(e.last_touched==((before^middle)|(middle^after)));
                assert(e.last_returned==(before & after & ~middle));
                assert(e.reference==after && e.phase==READY);
            }
    Engine e=baseline(7);
    hold(&e,7,5); hold(&e,6,5); hold(&e,7,5);
    hold(&e,15,104);
    assert(e.completed==0); /* five qualifiers then 100 complete intervals */
    hold(&e,15,1); assert(e.completed==1 && e.last_returned==1);
    /* A raw glitch must break closure stability even if qualification rejects it. */
    e=baseline(7); hold(&e,7,5); hold(&e,15,104);
    hold(&e,14,1); hold(&e,15,100); assert(e.completed==0);
    hold(&e,15,1); assert(e.completed==1 && e.last_touched==0);
    /* A lid glitch is not evidence of a qualified opening. */
    e=baseline(7); hold(&e,7,1); hold(&e,15,105);
    assert(e.aborted==1 && e.completed==0);
    hold(&e,15,105); assert(e.phase==READY);
    /* Sealed pocket disturbance cannot rewrite the reference. */
    e=baseline(7); hold(&e,14,1);
    assert(e.phase==HALT && e.fault==F_SEALED && e.reference==7);
    /* Deadline takes precedence over a completion on the same sample. */
    e=baseline(7); hold(&e,7,5); hold(&e,15,104);
    e.age=DEADLINE-1; hold(&e,15,1);
    assert(e.aborted==1 && e.completed==0 && e.phase==BASELINE);
    e=baseline(7); hold(&e,7,5);
    engine_sample(&e,e.now+2,7);
    assert(e.phase==HALT && e.aborted==1 && e.gaps==1 && engine_valid(&e));
    uint32_t seq=e.sequence; hold(&e,15,10); assert(e.sequence==seq);
    e=baseline(0); e.now=0xffffffffu; hold(&e,8,1); assert(e.phase==READY);
    e=baseline(0); e.starts=LIMIT; e.completed=LIMIT; e.last_age=109;
    hold(&e,0,1); assert(e.phase==HALT && e.fault==F_OVERFLOW);
    char buffer[768];
    assert(wire_crc("123456789",9)==0x29b1);
    assert(wire_encode(&e,buffer,sizeof buffer)>0);
    assert(wire_encode(&e,buffer,8)==0);
    e.starts=0; assert(!engine_valid(&e));
    assert(wire_encode(&e,buffer,sizeof buffer)==0);
    e=baseline(7); hold(&e,7,5); e.sequence=LIMIT;
    hold(&e,7,1);
    assert(e.phase==HALT && e.fault==F_OVERFLOW && e.aborted==1);
    assert(e.sequence==LIMIT && e.starts==1 && e.completed==0);
    e=baseline(7); hold(&e,7,5); e.tx_errors=LIMIT;
    engine_tx_error(&e);
    assert(engine_valid(&e) && e.phase==HALT && e.aborted==1);
    assert(e.tx_errors==LIMIT && e.fault==F_OVERFLOW);
    puts("PASS: 512 inventory paths; qualification, boundary, fault and CRC tests");
}
