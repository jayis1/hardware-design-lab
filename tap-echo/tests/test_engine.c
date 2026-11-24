/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, unsigned mask, unsigned duration)
{
    while (duration--) engine_sample(e, ++tick, mask);
}
static void tap(Engine *e, unsigned base, unsigned pad)
{
    hold(e, base | pad, 20);
    hold(e, base, 80);
}
static void record(Engine *e)
{
    hold(e, 4, 10); hold(e, 12, 10);
    tap(e, 12, 1); tap(e, 12, 2); hold(e, 4, 10);
}
int main(void)
{
    Engine e; engine_init(&e); tick = 0;
    record(&e);
    assert(e.references == 1 && e.reference_count == 2);
    assert(e.reference[0].pad == 1 && e.reference[1].pad == 2);
    assert(e.reference[1].interval == 100);
    hold(&e, 0, 10); hold(&e, 8, 10);
    tap(&e, 8, 1); tap(&e, 8, 2); hold(&e, 0, 10);
    assert(e.matches == 1 && e.trials == 1 && e.result == RESULT_MATCH);
    hold(&e, 8, 10); tap(&e, 8, 2); tap(&e, 8, 1); hold(&e, 0, 10);
    assert(e.trials == 2 && e.matches == 1 && e.result == RESULT_MISMATCH);
    hold(&e, 8, 10); tap(&e, 8, 1); hold(&e, 8, 200);
    tap(&e, 8, 2); hold(&e, 0, 10);
    assert(e.result == RESULT_MISMATCH && e.max_error == 200);
    unsigned aborted = e.aborted;
    hold(&e, 12, 10); hold(&e, 15, 1); hold(&e, 0, 10);
    assert(e.aborted == aborted + 1 && e.reference_count == 2);
    hold(&e, 8, 10); engine_sample(&e, tick += 5, 8);
    assert(e.missed == 4 && !e.active);
    hold(&e, 0, 10); hold(&e, 8, 10); hold(&e, 12, 1);
    assert(!e.active); hold(&e, 0, 10);
    hold(&e, 8, 10);
    for (unsigned i = 0; i < 17; ++i) tap(&e, 8, 1);
    assert(!e.active); hold(&e, 0, 10);
    hold(&e, 8, 10); hold(&e, 8, DEADLINE + 1);
    assert(e.timeouts == 1 && !e.active); hold(&e, 0, 10);
    assert(engine_valid(&e));
    char frame[1024];
    size_t n = wire_encode(&e, frame, sizeof frame);
    assert(n && !memcmp(frame, "TE1,", 4));
    assert(!wire_encode(&e, frame, 5));
    assert(wire_crc("123456789", 9) == 0x29b1);
    engine_init(&e); tick = UINT32_MAX - 20;
    hold(&e, 0, 10); record(&e);
    assert(e.references == 1 && e.missed == 0);
    engine_fault(&e, F_CONFIG); record(&e); assert(e.references == 1);
    engine_init(&e); tick = 0; hold(&e, 8, 10); hold(&e, 0, 10);
    assert(!e.references && !e.trials);
    e.samples = UINT32_MAX; hold(&e, 0, 1);
    assert(e.fault & F_OVERFLOW);
    engine_init(&e); tick = 0; record(&e);
    hold(&e, 0, 10); hold(&e, 8, 10);
    hold(&e, 9, 1); hold(&e, 10, 10);
    assert(!e.active); /* Raw A-to-B envelope without qualified release. */
    puts("PASS engine capture, replay, rejection, wrap, CRC, bounds");
}
