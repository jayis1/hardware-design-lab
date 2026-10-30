/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static void hold(Engine *e, unsigned mask, unsigned ticks)
{
    while (ticks--) {
        engine_sample(e, ++clock_ms, (uint8_t)mask);
        assert(engine_valid(e));
    }
}
static void fresh(Engine *e, unsigned mask)
{
    clock_ms = 0;
    engine_init(e);
    assert(engine_valid(e));
    hold(e, mask, 20);
}
static void emit(Engine *e)
{
    char out[512];
    assert(wire_encode(e, out, sizeof out));
    fputs(out, stdout);
}
int main(int argc, char **argv)
{
    Engine e;
    fresh(&e, 5);
    assert(e.state == READY && e.anchor == 1);
    hold(&e, 4, 100);
    assert(e.state == ACTIVE && e.started == 1);
    hold(&e, 6, 20);
    assert(e.full_a == 1 && e.ms_a == 100 && e.state == READY);
    hold(&e, 4, 80);
    hold(&e, 5, 20);
    assert(e.full_a == 2 && e.ms_a == 180);
    if (argc > 1 && !strcmp(argv[1], "fixture")) {
        emit(&e);
        hold(&e, 5, 100);
        emit(&e);
        hold(&e, 4, 100);
        emit(&e);
        hold(&e, 6, 100);
        emit(&e);
        return 0;
    }
    for (unsigned face = 0; face < 2; ++face) {
        for (unsigned side = 1; side <= 2; ++side) {
            unsigned f = face * 8;
            fresh(&e, 4 | f | side);
            hold(&e, 4 | f, 30);
            hold(&e, 4 | f | (3-side), 20);
            assert(e.full_a == !face && e.full_b == face);
            assert(e.aborted == 0);
        }
    }
    fresh(&e, 4); /* Attachment in mid-travel cannot invent departure. */
    assert(e.state == WAIT);
    hold(&e, 6, 20);
    assert(e.state == READY && !e.started);
    hold(&e, 4, 20);
    hold(&e, 6, 20);
    assert(e.short_returns == 1 && e.aborted == 1);
    fresh(&e, 5);
    hold(&e, 4, 20);
    hold(&e, 0, 20);
    assert(e.lifts == 1 && e.aborted == 1 && e.state == WAIT);
    fresh(&e, 5);
    hold(&e, 4, 20);
    hold(&e, 14, 20); /* Face change at arrival does not earn a stroke. */
    assert(e.flips == 1 && e.aborted == 1 && !e.full_b);
    fresh(&e, 5);
    hold(&e, 4, 20);
    hold(&e, 7, 20);
    assert(e.collisions == 1 && e.aborted == 1);
    hold(&e, 7, 20);
    assert(e.collisions == 1);
    fresh(&e, 5);
    hold(&e, 4, QUALIFY_MS);
    uint32_t start = e.departed;
    while (clock_ms - start < MAX_STROKE_MS - QUALIFY_MS)
        hold(&e, 4, 1);
    hold(&e, 6, QUALIFY_MS);
    assert(e.timeouts == 1 && !e.full_a); /* Exact deadline loses. */
    fresh(&e, 5);
    hold(&e, 4, 20);
    engine_sample(&e, clock_ms += 2, 6);
    assert(e.gaps == 1 && e.aborted == 1 && !e.known);
    hold(&e, 6, 20);
    assert(e.state == READY && !e.full_a);
    fresh(&e, 5);
    hold(&e, 4, 3); /* Rejected glitch. */
    hold(&e, 5, 20);
    assert(!e.started);
    fresh(&e, 5);
    for (unsigned i = 0; i < 20; ++i)
        hold(&e, 4 | ((i & 1) ? 8 : 0), 1);
    assert(e.state == ACTIVE); /* Unrelated face chatter cannot hide departure. */
    fresh(&e, 5);
    e.now = clock_ms = UINT32_MAX - 15;
    hold(&e, 4, 20);
    hold(&e, 6, 20);
    assert(e.full_a == 1 && !e.gaps);
    fresh(&e, 5);
    e.started = e.aborted = UINT32_MAX;
    hold(&e, 4, 20);
    assert(e.state == HALT && (e.fault & F_OVERFLOW));
    fresh(&e, 5);
    hold(&e, 4, 20);
    e.tx = UINT32_MAX;
    engine_tx_error(&e);
    assert(e.state == HALT && e.aborted == 1 && engine_valid(&e));
    fresh(&e, 5);
    engine_sample(&e, ++clock_ms, 16);
    assert(e.state == HALT && e.fault == F_INPUT && engine_valid(&e));
    char out[512];
    assert(!wire_encode(&e, out, 4));
    e.full_a = 1;
    assert(!wire_encode(&e, out, sizeof out));
    /* Deterministic adversarial masks exercise conservation on every tick. */
    fresh(&e, 0);
    unsigned random = 12345;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        hold(&e, random >> 28, 11);
    }
    puts("PASS engine: directions, faces, faults, timeout, wrap, exhaustion, fuzz");
    return 0;
}
