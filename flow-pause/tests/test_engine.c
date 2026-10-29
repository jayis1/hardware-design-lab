/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static void hold(Engine *e, uint8_t mask, unsigned n)
{
    while (n--) { engine_sample(e, e->now + 1u, mask); assert(engine_valid(e)); }
}
static void start(Engine *e)
{
    engine_init(e); hold(e, 2, 12); assert(e->state == READY);
    hold(e, 10, 12); assert(e->state == WATCH && e->runs == 1);
}
static void pulse(Engine *e)
{
    hold(e, 11, 12); hold(e, 10, 12);
}
int main(int argc, char **argv)
{
    Engine e;
    start(&e); pulse(&e); assert(e.pulses == 1);
    hold(&e, 10, SILENCE_MS); assert(e.state == PAUSED && e.pauses == 1);
    hold(&e, 14, 12); hold(&e, 10, 12); assert(e.taps == 1);
    pulse(&e); assert(e.recovered == 1 && e.assisted == 1 && e.state == WATCH);
    hold(&e, 2, 12); assert(e.finished == 1 && e.state == READY);
    if (argc > 1 && !strcmp(argv[1], "fixture")) {
        char text[768]; assert(wire_encode(&e, text, sizeof text)); fputs(text, stdout); return 0;
    }
    /* Startup with enable held is never permission to start. */
    engine_init(&e); hold(&e, 10, 100); assert(!e.runs && e.state == WAIT);
    hold(&e, 2, 12); hold(&e, 10, 12); assert(e.runs == 1);
    /* Independent qualification: unrelated tap chatter cannot hide supply loss. */
    for (unsigned i = 0; i < 20; ++i) hold(&e, 8 | ((i & 1) << 2), 1);
    assert(e.aborted == 1 && e.state == WAIT);
    start(&e); hold(&e, 11, MAX_PULSE_MS + 20);
    assert(e.bad_pulses == 1 && e.aborted == 1 && !e.pulses);
    /* Any gap abandons the current observation; no invented elapsed silence. */
    start(&e); hold(&e, 10, SILENCE_MS); assert(e.state == PAUSED);
    engine_sample(&e, e.now + 2, 10); assert(engine_valid(&e));
    assert(e.gaps == 1 && e.aborted == 1 && e.abandoned == 1);
    hold(&e, 10, 20); assert(e.state == WAIT);
    /* Duplicate sample is a gap too, including across wrap. */
    start(&e); engine_sample(&e, e.now, 10); assert(e.aborted == 1);
    engine_init(&e); e.now = UINT32_MAX - 10; e.sampled = true;
    hold(&e, 2, 20); hold(&e, 10, 20); assert(e.runs == 1 && !e.gaps);
    /* Exact silence boundary enters pause, but a completing pulse wins. */
    start(&e); e.silent = SILENCE_MS - 1; hold(&e, 10, 1); assert(e.state == PAUSED);
    start(&e); hold(&e, 11, 10); hold(&e, 10, QUALIFY_MS - 1);
    e.silent = SILENCE_MS - 1; hold(&e, 10, 1);
    assert(e.pulses == 1 && e.state == WATCH && !e.pauses);
    /* Counter exhaustion freezes meaningful evidence and conserves runs. */
    start(&e); e.pulses = UINT32_MAX;
    hold(&e, 11, 12); hold(&e, 10, 12);
    assert(e.fault & F_OVERFLOW); assert(e.state == HALT && e.aborted == 1);
    assert(e.pulses == UINT32_MAX && engine_valid(&e));
    /* Exhaustion while paused must account its abandonment exactly once. */
    start(&e); hold(&e, 10, SILENCE_MS); e.taps = UINT32_MAX;
    hold(&e, 14, 12); assert(e.state == HALT && e.abandoned == 1);
    assert(engine_valid(&e));
    /* Raw subqualification pulse must not create discharge. */
    start(&e); hold(&e, 11, 3); hold(&e, 10, 20); assert(!e.pulses);
    /* A simultaneous intervention/recovery cannot count as prior help. */
    start(&e); hold(&e, 10, SILENCE_MS); hold(&e, 11, 12);
    hold(&e, 14, 12); assert(e.recovered == 1 && !e.assisted && !e.taps);
    /* Release exactly at the maximum width is rejected, not counted. */
    start(&e); hold(&e, 11, QUALIFY_MS);
    hold(&e, 11, MAX_PULSE_MS - QUALIFY_MS);
    hold(&e, 10, QUALIFY_MS);
    assert(e.bad_pulses == 1 && !e.pulses && e.aborted == 1);
    /* Transport diagnostics cannot wrap or rewrite already halted totals. */
    start(&e); e.tx = UINT32_MAX; engine_tx_error(&e);
    assert(e.state == HALT && e.aborted == 1 && engine_valid(&e));
    /* All masks and random gaps maintain conservation. */
    engine_init(&e); unsigned rng = 1;
    for (unsigned i = 0; i < 100000; ++i) {
        rng = rng * 1664525u + 1013904223u;
        hold(&e, (rng >> 24) & 15, 10);
        if (i % 127 == 0) { engine_sample(&e, e.now + 3, 0); assert(engine_valid(&e)); }
    }
    char text[768]; assert(wire_encode(&e, text, sizeof text));
    assert(!wire_encode(&e, text, 8)); assert(wire_crc("123456789",9) == 0x29b1);
    e.assisted = e.recovered + 1; assert(!engine_valid(&e));
    assert(!wire_encode(&e, text, sizeof text));
    puts("PASS flow-pause engine boundaries, gaps, overflow, random masks, wire CRC");
}
