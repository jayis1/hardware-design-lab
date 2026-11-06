/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, unsigned mask)
{
    for (unsigned i=0; i<QUALIFY; ++i) {
        engine_sample(e, ++now, (uint8_t)mask);
        assert(engine_valid(e));
    }
}
static void baseline(Engine *e)
{
    now=0;
    engine_init(e);
    hold(e,0);
    assert(e->state==RUNNING);
}
/* Independent oracle: array of cohorts, counting older members directly.
 * Every four-vector word includes rolling reuse and tied changes. */
static void exhaustive(void)
{
    for (unsigned word=0; word<65536; ++word) {
        Engine e;
        baseline(&e);
        unsigned stamp[4]={0}, histogram[4]={0};
        unsigned epoch=0, ins=0, rem=0, amb=0;
        for (unsigned step=0; step<4; ++step) {
            unsigned mask=(word>>(step*4))&15;
            unsigned old[4];
            memcpy(old,stamp,sizeof old);
            unsigned lo[4]={0}, hi[4]={0}, removed=0, added=0;
            for (unsigned i=0;i<4;++i) {
                if (old[i] && !(mask&(1u<<i))) {
                    removed|=1u<<i;
                    for (unsigned j=0;j<4;++j) {
                        if (old[j] && old[j]<old[i]) ++lo[i];
                        if (j!=i && old[j] && old[j]<=old[i]) ++hi[i];
                    }
                    ++rem;
                    if(lo[i]==hi[i]) ++histogram[lo[i]];
                    else ++amb;
                    stamp[i]=0;
                }
                if (!old[i] && (mask&(1u<<i))) added|=1u<<i;
            }
            if (added) ++epoch;
            for (unsigned i=0;i<4;++i)
                if (added&(1u<<i)) { stamp[i]=epoch; ++ins; }
            hold(&e,mask);
            assert(e.state==RUNNING && e.occupied==mask);
            assert(e.epoch==epoch && e.insertions==ins && e.removals==rem);
            assert(e.ambiguous==amb);
            for (unsigned i=0;i<4;++i) {
                assert(e.stamp[i]==stamp[i] && e.rank[i]==histogram[i]);
                if(removed) assert(e.low[i]==lo[i] && e.high[i]==hi[i]);
            }
            if(removed) assert(e.last_mask==removed);
        }
    }
}
static void edges(void)
{
    Engine e;
    engine_init(&e); now=0;
    hold(&e,15);
    assert(e.state==WAIT && e.insertions==0);
    hold(&e,0); assert(e.state==RUNNING);
    hold(&e,3); hold(&e,2);
    assert(e.ambiguous==1 && e.low[0]==0 && e.high[0]==1);
    hold(&e,0); assert(e.rank[0]==1);
    baseline(&e);
    /* Four short samples are insufficient; unrelated chatter is independent. */
    for(unsigned i=0;i<4;++i) engine_sample(&e,++now,1);
    assert(e.insertions==0);
    engine_sample(&e,++now,1); assert(e.insertions==1);
    for(unsigned i=0;i<10;++i) engine_sample(&e,++now,(i&1)?3:1);
    assert(e.insertions==1);
    engine_sample(&e,now+2,1);
    assert(e.state==HALT && e.fault==F_GAP && e.abandoned==1 && !e.occupied);
    assert(engine_valid(&e));
    uint32_t seq=e.sequence;
    engine_sample(&e,now+5,15); assert(e.sequence==seq && e.insertions==1);
    baseline(&e); hold(&e,1);
    e.sequence=UINT32_MAX;
    hold(&e,0);
    assert(e.fault==F_COUNTER && e.removals==0 && e.abandoned==1);
    /* A two-insertion sample with one ticket left must roll back the first
     * insertion as well; preserve an internally consistent prior ledger. */
    baseline(&e); hold(&e,1); hold(&e,0);
    e.insertions=LIMIT-1; e.removals=LIMIT-1;
    e.epoch=LIMIT-1; e.rank[0]=LIMIT-1;
    assert(engine_valid(&e));
    hold(&e,3);
    assert(e.fault==F_COUNTER && e.insertions==LIMIT-1 && !e.abandoned);
    assert(e.removals==LIMIT-1 && !e.occupied && engine_valid(&e));
    baseline(&e); hold(&e,1); hold(&e,0);
    e.insertions=LIMIT-1; e.removals=LIMIT-1;
    e.epoch=LIMIT-1; e.rank[0]=LIMIT-1;
    hold(&e,1); hold(&e,0);
    assert(e.state==RUNNING && e.insertions==LIMIT && e.removals==LIMIT);
    hold(&e,1); assert(e.fault==F_COUNTER && engine_valid(&e));
    baseline(&e);
    engine_sample(&e,++now,16); assert(e.fault==F_CONFIG);
    engine_init(&e); now=UINT32_MAX-2;
    hold(&e,0); assert(e.state==RUNNING);
    char frame[768];
    assert(wire_crc("123456789",9)==0x29b1);
    size_t size=wire_encode(&e,frame,sizeof frame);
    assert(size && !wire_encode(&e,frame,size-1));
    assert(!wire_encode(&e,NULL,sizeof frame));
    e.rank[0]=1; assert(!engine_valid(&e));
    baseline(&e);
    e.tx_errors=LIMIT; engine_tx_error(&e); assert(e.tx_errors==LIMIT);
    engine_fault(&e,F_WATCHDOG); assert(engine_valid(&e));
}
int main(void)
{
    exhaustive(); edges();
    puts("PASS 65536 four-mask words, rank intervals, ties, reuse, startup, gap, wrap, overflow, encoder");
}
