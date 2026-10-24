/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, unsigned mask, unsigned duration)
{
    for (unsigned i = 0; i < duration; ++i)
        engine_sample(e, ++now, (uint8_t)mask);
}
static void setup(Engine *e)
{
    now = 0;
    engine_init(e);
    hold(e, 3, 1100);
    assert(e->phase == PH_ARMED);
}
int main(int argc, char **argv)
{
    Engine e;
    setup(&e);
    hold(&e, 1, 100);
    assert(e.phase == PH_TRANSFER);
    hold(&e, 5, 100);
    hold(&e, 4, 100);
    hold(&e, 12, 100);
    assert(e.phase == PH_COMPLETE);
    assert(e.cycles == 1 && e.failed == 0);
    assert(e.drain_ms == 200 && e.fill_ms == 200);
    char frame[256];
    size_t n = wire_encode(&e, now, 0, frame, sizeof frame);
    assert(n > 0 && n < sizeof frame);
    assert(wire_encode(&e, now, 0, frame, 5) == 0);
    n = wire_encode(&e, now, 0, frame, sizeof frame);
    if (argc > 1 && strcmp(argv[1], "--fixture") == 0)
        fwrite(frame, 1, n, stdout);
    hold(&e, 3, 1100);
    assert(e.phase == PH_ARMED && e.trial == 2);
    hold(&e, 1, 100);
    hold(&e, 3, 100);
    assert(e.phase == PH_FAULT && (e.flags & FL_REVERSE));
    assert(e.failed == 1);
    setup(&e);
    hold(&e, 2, 100);
    assert(e.phase == PH_FAULT && (e.flags & FL_TOPOLOGY));
    setup(&e);
    engine_sample(&e, now + 10, 3);
    assert(e.phase == PH_FAULT && (e.flags & FL_GAP));
    assert(e.gaps == 1);
    setup(&e);
    hold(&e, 3, TRIAL_LIMIT_MS + 1);
    assert(e.phase == PH_FAULT && (e.flags & FL_TIMEOUT));
    setup(&e);
    hold(&e, 0, 100);
    assert(e.phase == PH_FAULT && (e.flags & FL_SKIPPED));
    setup(&e);
    hold(&e, 1, 10);
    hold(&e, 3, 100);
    assert(e.phase == PH_ARMED);
    engine_init(&e);
    now = UINT32_MAX - 1200;
    hold(&e, 3, 1300);
    assert(e.phase == PH_ARMED);
    hold(&e, 1, 100);
    hold(&e, 5, 100);
    hold(&e, 4, 100);
    hold(&e, 12, 100);
    assert(e.cycles == 1);
    assert(engine_sat_add(UINT32_MAX - 1, 3) == UINT32_MAX);
    engine_transport_loss(&e);
    assert(e.drops == 1 && (e.flags & FL_TRANSPORT));
    assert(wire_crc("123456789", 9) == 0x29b1);
    if (argc == 1)
        puts("PASS engine: completion, reverse, topology, gap, timeout, skipped band, bounce, wrap, saturation, transport, CRC, bounds");
    return 0;
}
