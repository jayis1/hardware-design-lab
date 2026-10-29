/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void ticks(Engine *e, uint8_t mask, unsigned n)
{
    while (n--) engine_sample(e, ++now, mask);
    assert(engine_valid(e));
}
static Engine fresh(void)
{
    Engine e;
    now = 0;
    engine_init(&e);
    ticks(&e, 0, 12);
    assert(e.slot[0].state == READY);
    return e;
}
int main(int argc, char **argv)
{
    Engine e = fresh();
    ticks(&e, 1, 9);
    assert(e.slot[0].starts == 1 && e.slot[0].age == 0);
    ticks(&e, 1, 100);
    ticks(&e, 0, 9);
    assert(e.slot[0].early == 1 && e.slot[0].last == 109);
    assert(e.slot[1].starts == 0);
    ticks(&e, 3, 9);
    ticks(&e, 3, GOAL_MS);
    ticks(&e, 0, 9);
    assert(e.slot[0].rested == 1 && e.slot[1].rested == 1);
    assert(e.slot[0].minimum == 109);
    assert(e.slot[0].total == e.slot[0].last + 109);
    if (argc > 1 && !strcmp(argv[1], "fixture")) {
        char b[768];
        assert(wire_encode(&e, b, sizeof b));
        fputs(b, stdout);
        ticks(&e, 0, 100);
        assert(wire_encode(&e, b, sizeof b));
        fputs(b, stdout);
        return 0;
    }
    e = fresh();
    /* Independent qualification: B chatter cannot hide A arrival. */
    for (unsigned i=0; i<30; ++i) ticks(&e, 1 | ((i&1)<<1), 1);
    assert(e.slot[0].starts == 1 && e.slot[1].starts == 0);
    engine_sample(&e, now += 5, 1);
    assert(e.gaps == 1 && e.slot[0].aborted == 1);
    ticks(&e, 1, 20);
    assert(e.slot[0].state == WAIT && e.slot[0].starts == 1);
    ticks(&e, 0, 9);
    ticks(&e, 1, 9);
    assert(e.slot[0].starts == 2);
    e = fresh();
    ticks(&e, 1, 9);
    ticks(&e, 1, LIMIT_MS);
    assert(e.slot[0].timeouts == 1 && e.slot[0].state == WAIT);
    ticks(&e, 1, 20);
    assert(e.slot[0].starts == 1);
    ticks(&e, 0, 9);
    assert(e.slot[0].state == READY);
    /* Exact boundary: age at qualified removal determines classification. */
    e = fresh();
    ticks(&e, 1, 9);
    ticks(&e, 1, GOAL_MS-9);
    ticks(&e, 0, 9);
    assert(e.slot[0].last == GOAL_MS && e.slot[0].rested == 1);
    e = fresh();
    ticks(&e, 1, 9);
    ticks(&e, 1, GOAL_MS-10);
    ticks(&e, 0, 9);
    assert(e.slot[0].last == GOAL_MS-1 && e.slot[0].early == 1);
    /* Ordinary uptime wrap must not create gaps. */
    e = fresh();
    e.now = now = UINT32_MAX-5;
    ticks(&e, 0, 20);
    assert(e.gaps == 0);
    /* Transactional overflow leaves no half-recorded completion. */
    e = fresh();
    e.slot[0].starts = e.slot[0].early = UINT32_MAX;
    e.slot[0].minimum = e.slot[0].maximum = e.slot[0].last = 1;
    e.slot[0].total = UINT32_MAX;
    assert(engine_valid(&e));
    ticks(&e, 1, 9);
    assert(e.fault == F_OVERFLOW && e.slot[0].starts == UINT32_MAX);
    /* Closed at boot is unknown, but other empty pockets recover separately. */
    engine_init(&e);
    now = 0;
    ticks(&e, 1, 40);
    assert(e.slot[0].state == WAIT && e.slot[0].starts == 0);
    assert(e.slot[1].state == READY);
    ticks(&e, 3, 9);
    assert(e.slot[1].starts == 1 && e.slot[0].starts == 0);
    /* Total overflow must abandon the existing placement, not finish it. */
    e = fresh();
    e.slot[0].starts = e.slot[0].early = UINT32_MAX;
    e.slot[0].starts--;
    e.slot[0].early--;
    e.slot[0].minimum = e.slot[0].maximum = e.slot[0].last = 1;
    e.slot[0].total = UINT32_MAX-1;
    ticks(&e, 1, 9);
    ticks(&e, 1, 20);
    ticks(&e, 0, 9);
    assert(e.fault == F_OVERFLOW && e.slot[0].aborted == 1);
    assert(e.slot[0].total == UINT32_MAX-1 && e.slot[0].last == 1);
    /* Deterministic independent duration oracle for non-overlapping trials. */
    e = fresh();
    uint32_t seed = 17, expected[4] = {0}, counts[4] = {0};
    for (unsigned trial=0; trial<400; ++trial) {
        seed = seed*1664525u + 1013904223u;
        unsigned channel = (seed >> 24) & 3;
        unsigned hold = (seed >> 8) % 2000;
        ticks(&e, 1u << channel, 9);
        ticks(&e, 1u << channel, hold);
        ticks(&e, 0, 9);
        expected[channel] += hold+9;
        counts[channel]++;
        for (unsigned i=0;i<4;++i) {
            assert(e.slot[i].total == expected[i]);
            assert(e.slot[i].early == counts[i]);
        }
    }
    e = fresh();
    ticks(&e, 15, 9);
    engine_fault(&e, F_CONFIG);
    for (unsigned i=0;i<4;++i) assert(e.slot[i].aborted == 1);
    assert(engine_valid(&e));
    char b[768];
    assert(wire_encode(&e,b,sizeof b));
    assert(!wire_encode(&e,b,10));
    assert(wire_crc("123456789",9) == 0x29b1);
    e.slot[0].early++;
    assert(!engine_valid(&e) && !wire_encode(&e,b,sizeof b));
    puts("engine boundaries, gaps, independence, overflow, CRC: PASS");
}
