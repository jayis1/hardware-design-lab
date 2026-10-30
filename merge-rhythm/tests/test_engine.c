/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static uint32_t now;
static void hold(unsigned mask, unsigned n)
{
    while (n--) engine_sample(&e, ++now, (uint8_t)mask);
    assert(engine_valid(&e));
}
static void baseline(void)
{
    engine_init(&e); now = 0;
    hold(0, 12); hold(8, 120); hold(0, 12);
    assert(e.state == ARMED && e.segments == 1);
}
static void pulse(unsigned bit)
{
    hold(bit, 12); hold(0, 12);
}
static void deliver(unsigned bit)
{
    pulse(bit); pulse(4);
}
int main(int argc, char **argv)
{
    baseline();
    if (argc == 2 && strcmp(argv[1], "--fixture") == 0) {
        char line[768];
        for (unsigned i = 0; i < 8; ++i) {
            deliver(i % 3 ? 2 : 1);
            size_t n = wire_encode(&e, line, sizeof line);
            assert(n); fwrite(line, 1, n, stdout);
        }
        return 0;
    }
    assert(e.admitted == 0);
    deliver(1); deliver(2); deliver(2); deliver(1);
    assert(e.left == 2 && e.right == 2 && e.alternations == 2);
    assert(e.longest == 2 && e.streak == 1);
    baseline(); pulse(1); pulse(2); pulse(4); pulse(4);
    assert(e.left == 1 && e.right == 1 && e.pending == 0);
    baseline(); pulse(3);
    assert(e.state == WAIT && e.ambiguous == 1 && e.rejected == 2);
    baseline(); pulse(4);
    assert(e.state == WAIT && e.orphans == 1);
    baseline(); pulse(1); hold(6, 12); hold(0, 12); pulse(4);
    assert(e.left == 1 && e.right == 1); /* retire old before append */
    baseline(); hold(5,12);
    assert(e.state == WAIT && e.orphans == 1 && e.rejected == 1);
    baseline();
    for (unsigned i = 0; i < QUEUE_CAP; ++i) pulse(1);
    pulse(2);
    assert(e.state == WAIT && e.full == 1 && e.abandoned == QUEUE_CAP);
    baseline(); pulse(1);
    hold(0, TIMEOUT_MS);
    assert(e.state == WAIT && e.timeouts == 1 && e.abandoned == 1);
    baseline(); pulse(1); engine_sample(&e, now += 5, 0);
    assert(e.state == WAIT && e.gaps == 1 && e.abandoned == 1 && e.known != 15);
    hold(0,20); hold(8,120); hold(0,12); deliver(2);
    assert(e.right == 1 && e.segments == 2 && e.alternations == 0);
    baseline(); pulse(1); hold(8,12);
    assert(e.state == WAIT && e.abandoned == 1);
    hold(8,200); assert(e.state == WAIT); /* release is mandatory */
    hold(0,12); hold(8,120); assert(e.state == ARMED);
    baseline(); hold(1,3); hold(0,12); assert(e.admitted == 0);
    baseline(); hold(8,12); hold(0,12); hold(8,120);
    assert(e.state==ARMED); hold(9,12);
    assert(e.state==WAIT && e.rejected==1);
    baseline(); pulse(1);
    e.entered[e.head]=now-(TIMEOUT_MS-8);
    hold(4,8); /* qualified exit at the exact deadline loses correlation */
    assert(e.state==WAIT && e.timeouts==1 && e.left==0);
    baseline(); pulse(1);
    e.entered[e.head]=now-(TIMEOUT_MS-9);
    hold(4,8); assert(e.left==1 && e.latency_max==TIMEOUT_MS-1);
    baseline(); pulse(1);
    e.latency_sum=COUNTER_MAX; e.left=COUNTER_MAX/1000;
    e.admitted=e.left+1; e.longest=e.left; e.streak=e.left;
    e.last=0; e.latency_max=1000;
    assert(engine_valid(&e));
    pulse(4); assert(e.state==HALT && e.abandoned==1);
    assert(e.left==COUNTER_MAX/1000); /* transaction rolled back */
    baseline();
    for(unsigned i=0;i<QUEUE_CAP;++i) pulse(1);
    hold(6,8); assert(e.state==ARMED && e.pending==QUEUE_CAP && !e.full);
    assert(e.left==1 && e.admitted==QUEUE_CAP+1);
    baseline(); pulse(1);
    e.admitted=COUNTER_MAX; e.abandoned=COUNTER_MAX-2;
    e.left=1; e.latency_sum=1; e.latency_max=1;
    e.longest=1; e.streak=1; e.last=0;
    assert(engine_valid(&e)); hold(6,8);
    assert(e.state==HALT && e.left==1 && e.abandoned==COUNTER_MAX-1);
    assert(e.pending==0 && e.admitted==COUNTER_MAX && e.latency_sum==1);
    baseline();
    for (unsigned i=0;i<12;++i) hold(1 | ((i&1) ? 2:0),1);
    assert(e.admitted == 1); /* unrelated chatter must not hide left */
    baseline(); now = UINT32_MAX - 20; e.now = now;
    deliver(1); assert(e.left == 1 && !e.gaps);
    baseline(); e.tx = COUNTER_MAX; engine_tx_error(&e);
    assert(e.state == HALT && (e.fault & F_OVERFLOW));
    assert(engine_valid(&e));
    baseline(); e.segments = COUNTER_MAX; pulse(8); hold(0,12); hold(8,120);
    assert(e.state == HALT && engine_valid(&e));
    baseline(); pulse(1); e.tx=COUNTER_MAX; engine_tx_error(&e);
    assert(e.abandoned==1 && e.pending==0 && engine_valid(&e));
    char frame[768]; assert(wire_encode(&e,frame,sizeof frame));
    assert(!wire_encode(&e,frame,8));
    Engine bad=e; bad.left=1; assert(!engine_valid(&bad));
    assert(!wire_encode(&bad,frame,sizeof frame));
    /* Every pair of source labels through a multi-item queue. */
    for (unsigned pattern=0;pattern<256;++pattern) {
        baseline(); unsigned l=0,r=0,alt=0,streak=0,longest=0,last=2;
        for(unsigned i=0;i<8;++i) pulse((pattern&(1u<<i))?2:1);
        for(unsigned i=0;i<8;++i) {
            unsigned tag=(pattern>>i)&1u;
            if(tag) ++r; else ++l;
            if(last==tag) ++streak;
            else { if(last!=2) ++alt; streak=1; }
            if(streak>longest) longest=streak;
            last=tag; pulse(4);
        }
        assert(e.left==l && e.right==r && e.alternations==alt && e.longest==longest);
    }
    puts("PASS queue ordering, 256 patterns, ambiguity, rearm, timeouts, gaps, wrap, overflow and wire bounds");
    return 0;
}
