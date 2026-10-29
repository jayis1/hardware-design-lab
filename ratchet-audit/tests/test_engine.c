/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static uint32_t tick;
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) engine_sample(e, ++tick, (uint8_t)mask);
}
static void ready(Engine *e)
{
    tick = 0;
    engine_init(e);
    hold(e, 12, 120);
    assert(e->phase == READY);
}
static void forward(Engine *e)
{
    hold(e, 9, 12); hold(e, 11, 12);
    hold(e, 10, 12); hold(e, 8, 12);
}
int main(int argc, char **argv)
{
    Engine e;
    char frame[512];
    engine_init(&e);
    e.starts = e.completed = 1;
    e.reverse = 2;
    e.rollback = 1;
    assert(!engine_valid(&e));
    engine_init(&e);
    e.starts = 1;
    e.phase = ACTIVE;
    e.duration = TRIAL_LIMIT;
    assert(!engine_valid(&e));
    tick = 0;
    engine_init(&e);
    hold(&e, 15, 120);
    assert(e.phase == READY && e.faults == 0);
    ready(&e);
    hold(&e, 8, 12);
    assert(e.phase == ACTIVE && e.starts == 1);
    forward(&e);
    hold(&e, 10, 12); hold(&e, 11, 12);
    assert(e.forward == 4 && e.reverse == 2);
    assert(e.seated_reverse == 2 && e.rollback == 2);
    hold(&e, 15, 12);
    assert(e.phase == WAIT && e.completed == 1 && e.aborted == 0);
    assert(engine_valid(&e));
    assert(wire_encode(&e, frame, sizeof frame));
    if (argc > 1 && !strcmp(argv[1], "fixture")) fputs(frame, stdout);
    hold(&e, 15, 120);
    assert(e.phase == READY && e.forward == 4);
    hold(&e, 11, 12);
    assert(e.phase == ACTIVE && e.forward == 0);
    engine_sample(&e, tick += 3, 11);
    assert(e.phase == WAIT && e.aborted == 1 && e.gaps == 1);
    assert(e.starts == e.completed + e.aborted);
    ready(&e); hold(&e, 8, 12); hold(&e, 11, 12);
    assert(e.aborted == 1 && (e.faults & F_QUADRATURE));
    ready(&e); hold(&e, 8, 12); hold(&e, 8, TRIAL_LIMIT);
    assert(e.aborted == 1 && (e.faults & F_TIMEOUT));
    ready(&e);
    e.starts = e.completed = UINT32_MAX;
    hold(&e, 8, 12);
    assert(e.phase == HALTED && (e.faults & F_OVERFLOW));
    assert(engine_valid(&e));
    ready(&e); hold(&e, 8, 12);
    e.gaps = UINT32_MAX;
    engine_sample(&e, tick += 2, 8);
    assert(e.phase == HALTED && e.aborted == 1);
    assert(engine_valid(&e));
    ready(&e); hold(&e, 8, 12);
    tick = UINT32_MAX - 10; e.now = tick;
    hold(&e, 8, 20);
    assert(e.phase == ACTIVE && e.gaps == 0);
    /* Pawl opening and reverse edge in one qualified vector is not seated. */
    ready(&e); hold(&e, 8, 12);
    hold(&e, 2, 12);
    assert(e.reverse == 1 && e.seated_reverse == 0);
    /* Independent HOME qualification cannot be postponed by pawl chatter. */
    ready(&e);
    for (unsigned i = 0; i < 12; ++i) hold(&e, i & 1 ? 0 : 8, 1);
    assert(e.phase == ACTIVE);
    /* Too-small buffers never produce a nominally successful frame. */
    assert(wire_encode(&e, frame, 8) == 0);
    e.seated_reverse = e.reverse + 1;
    assert(!engine_valid(&e) && wire_encode(&e, frame, sizeof frame) == 0);
    /* Deterministic acquisition fuzz: transitions, chatter, gaps, and wrap
     * must always preserve invariants and a serializable snapshot. */
    engine_init(&e);
    uint32_t random = 1931;
    uint8_t mask = 12;
    tick = UINT32_MAX - 100;
    for (unsigned i = 0; i < 200000; ++i) {
        random = random * 1664525u + 1013904223u;
        if ((random & 63u) == 0)
            mask = (uint8_t)((random >> 24) & 15u);
        tick += (random & 4095u) == 0 ? 2u : 1u;
        engine_sample(&e, tick, mask);
        assert(engine_valid(&e));
        assert(wire_encode(&e, frame, sizeof frame));
    }
    if (argc == 1) puts("PASS ratchet trials, rollback, gaps, quadrature, timeout, exhaustion, wrap, independent qualification and wire bounds");
    return 0;
}
