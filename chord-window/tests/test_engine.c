/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static CW e;
static uint32_t clock_ms;
static void fixture(const char *name)
{
    char line[256];
    size_t n = cw_encode(&e, clock_ms, 0, line, sizeof line);
    assert(n);
    FILE *out = fopen(name, "w");
    assert(out);
    assert(fwrite(line, 1, n, out) == n);
    assert(!fclose(out));
}
static void hold(unsigned mask, unsigned ms)
{
    for (unsigned i = 0; i < ms; ++i)
        cw_tick(&e, ++clock_ms, (uint8_t)mask);
}
static void ready(void)
{
    clock_ms = 0;
    cw_init(&e, clock_ms);
    hold(0, 110);
    assert(e.state == CW_READY);
}
int main(void)
{
    ready();
    hold(1, 20); hold(3, 40); hold(2, 10); hold(0, 6);
    assert(e.result.id == 1 && e.result.reason == CW_OK);
    assert(e.result.keys == 3 && e.result.spread == 20);
    assert(e.result.overlap == 40 && e.result.release == 10);
    assert(e.result.duration == 70);
    hold(0, 110);
    hold(1, 10); hold(3, 10); hold(2, 10); hold(3, 10); hold(0, 6);
    assert(e.result.reason == CW_RETAP);
    assert(!e.result.spread && !e.result.overlap && !e.result.release);
    fixture("build/retap.txt");
    ready();
    hold(1, 10);
    clock_ms += 5;
    cw_tick(&e, clock_ms, 0);
    assert(e.result.reason == CW_GAP && e.gaps == 1);
    assert(!e.result.spread && !e.result.overlap && !e.result.release);
    fixture("build/gap.txt");
    assert(e.state == CW_WAIT);
    hold(0, 110); assert(e.state == CW_READY);
    ready(); hold(1, 10010);
    assert(e.result.reason == CW_TIMEOUT && e.state == CW_WAIT);
    assert(!e.result.spread && !e.result.overlap && !e.result.release);
    assert(e.result.duration == CW_LIMIT);
    fixture("build/timeout.txt");
    hold(1, 120); assert(e.state == CW_WAIT);
    hold(0, 110); assert(e.state == CW_READY);
    ready(); hold(1, 10000); hold(0, 6);
    assert(e.result.reason == CW_TIMEOUT);
    ready(); hold(1, 20); hold(2, 20); hold(0, 6);
    assert(e.result.reason == CW_OK && e.result.overlap == 0);
    assert(e.result.spread == 20 && e.result.release == 20);
    fixture("build/rolling.txt");
    ready();
    /* Unrelated chatter must not postpone A's qualification. */
    for (unsigned i = 0; i < 7; ++i)
        cw_tick(&e, ++clock_ms, (uint8_t)(1 | ((i & 1) << 1)));
    assert(e.mask == 1 && e.state == CW_ACTIVE);
    ready(); hold(1, 4); hold(0, 10); assert(!e.result.id);
    clock_ms = UINT32_MAX - 50; cw_init(&e, clock_ms);
    hold(0, 110); hold(3, 20); hold(0, 6);
    assert(e.result.reason == CW_OK && e.result.duration == 20);
    ready(); e.result.id = UINT32_MAX;
    hold(1, 10); hold(0, 6); assert(e.state == CW_FAULT);
    char line[256];
    ready(); hold(1, 20); hold(3, 40); hold(2, 10); hold(0, 6);
    size_t n = cw_encode(&e, clock_ms, 0, line, sizeof line);
    assert(n > 0 && line[n-1] == '\n');
    assert(!cw_encode(&e, clock_ms, 0, line, 5));
    assert(cw_crc("123456789", 9) == 0x29b1);
    puts("PASS engine: chord, retap, gap, timeout, chatter, glitch, wrap, exhaustion, codec");
    FILE *out = fopen("build/fixture.txt", "w");
    assert(out); n = cw_encode(&e, clock_ms, 0, line, sizeof line);
    assert(fwrite(line, 1, n, out) == n); fclose(out);
    return 0;
}
