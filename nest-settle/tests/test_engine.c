/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static NS e;
static uint32_t now;
static void hold(uint8_t raw, unsigned ms)
{
    while (ms--) ns_tick(&e, ++now, raw);
}
static void fresh(void)
{
    now = 0;
    ns_init(&e, now);
    hold(0, 110);
    assert(e.state == NS_READY);
}
int main(void)
{
    fresh();
    hold(15, 300);
    assert(e.state == NS_ACTIVE && e.settled);
    hold(14, 20);
    hold(15, 300);
    hold(0, 6);
    assert(e.result.id == 1 && e.result.reason == NS_OK);
    assert(e.result.drop_mask == 1 && e.result.drops == 1);
    assert(e.result.first_settle == 250);
    assert(e.result.full_ms >= 590 && e.result.longest >= 295);
    char frame[256];
    size_t n = ns_encode(&e, now, 0, frame, sizeof frame);
    assert(n && !memcmp(frame, "NS1,", 4));
    assert(!ns_encode(&e, now, 0, frame, 5));
    FILE *out = fopen("build/fixture.txt", "w");
    assert(out);
    n = ns_encode(&e, now, 0, frame, sizeof frame);
    assert(fwrite(frame, 1, n, out) == n); fclose(out);
    hold(0, 110); assert(e.state == NS_READY);
    hold(3, 50); hold(0, 6);
    assert(e.result.reason == NS_UNSETTLED);
    fresh(); hold(15, 200); hold(14, 3); hold(15, 60);
    assert(e.settled); /* subqualification glitch does not break qualified dwell */
    fresh(); hold(1, 10); hold(3, 2);
    assert(e.mask == 1); hold(3, 4); assert(e.mask == 3);
    ns_tick(&e, now += 10, 15);
    assert(e.result.reason == NS_GAP && e.state == NS_WAIT && e.gaps == 1);
    hold(15, 300); assert(e.state == NS_WAIT);
    hold(0, 110); assert(e.state == NS_READY);
    hold(1, NS_LIMIT + 10); assert(e.result.reason == NS_TIMEOUT);
    hold(0, 110); assert(e.state == NS_READY);
    now = UINT32_MAX - 50; ns_init(&e, now);
    hold(0, 110); hold(15, 300); hold(0, 6);
    assert(e.result.reason == NS_OK);
    fresh(); e.result.id = UINT32_MAX; hold(1, 20); hold(0, 6);
    assert(e.state == NS_FAULT);
    fresh(); hold(15, 100); ns_fault(&e); hold(0, 1000);
    assert(e.state == NS_FAULT);
    assert(ns_crc("123456789", 9) == 0x29b1);
    fresh(); hold(15, 250); hold(0, 6);
    assert(e.result.longest == 250 && e.result.reason == NS_UNSETTLED);
    fresh(); hold(15, NS_LIMIT + 6);
    assert(e.result.reason == NS_TIMEOUT && e.result.full_ms == NS_LIMIT);
    puts("PASS seating, drops, debounce, baseline, timeout, gap, rollover, exhaustion, CRC");
}
