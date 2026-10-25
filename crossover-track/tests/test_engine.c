/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static uint32_t now;
static void hold(uint8_t mask, uint32_t ms)
{
    while (ms--) engine_sample(&e, ++now, mask);
}
static void begin(uint32_t start)
{
    now = start;
    engine_init(&e, now);
    hold(5, 140);
    assert(e.phase == ARMED);
}
static void race(void)
{
    hold(4, 100);
    hold(0, 200);
    hold(2, 100);
    hold(10, 20);
}
int main(void)
{
    begin(0);
    race();
    assert(e.good == 1 && e.bad == 0);
    assert(e.last.ok && e.last.start[0] == 0 && e.last.start[1] == 100);
    assert(e.last.finish[0] == 300 && e.last.finish[1] == 400);
    assert(e.last.id == 1);
    hold(10, 1000);
    assert(e.trial == 1);
    hold(5, 140);
    assert(e.phase == ARMED);
    hold(0, 50);
    hold(10, 20);
    assert(e.good == 2);
    assert(e.last.finish[0] == e.last.finish[1]);
    begin(UINT32_MAX - 200);
    race();
    assert(e.last.finish[1] == 400);
    begin(0);
    hold(6, 20); /* left release and left finish together: ambiguous */
    assert(e.bad == 1 && e.last.reason == ORDER);
    begin(0);
    hold(4, 40);
    hold(5, 20); /* return to start during trial */
    assert(e.bad == 1 && e.last.reason == RETURNED);
    begin(0);
    hold(4, 2100);
    assert(e.bad == 1 && e.last.reason == SKEW);
    begin(0);
    hold(4, 2000);
    hold(0, 20);
    hold(10, 20);
    assert(e.good == 1 && e.last.start[1] == 2000);
    begin(0);
    hold(0, 29999);
    hold(10, 20);
    assert(e.good == 1 && e.last.finish[0] == 29999);
    begin(0);
    hold(0, 30000);
    hold(10, 20);
    assert(e.good == 1 && e.last.finish[0] == 30000);
    begin(0);
    hold(0, 30020);
    assert(e.bad == 1 && e.last.reason == TIMEOUT);
    begin(0);
    hold(0, 100);
    now += 10;
    engine_sample(&e, now, 10);
    assert(e.gaps == 1 && e.bad == 1 && e.last.reason == GAP);
    hold(5, 140);
    assert(e.phase == ARMED);
    begin(0);
    hold(4, 5); /* rejected glitch must not start */
    hold(5, 20);
    assert(e.trial == 0);
    begin(0);
    /* Independent qualification: right chatter cannot delay left release. */
    for (unsigned i=0; i<20; ++i) hold((i&1) ? 4 : 0, 1);
    assert(e.phase == RUNNING && e.started[0] && !e.started[1]);
    engine_transport_loss(&e);
    assert(e.loss == 1 && e.bad == 1 && e.last.reason == LINK);
    begin(0);
    e.trial = UINT32_MAX;
    e.phase = WAITING;
    e.baseline = 0;
    hold(5, 200);
    assert(e.phase == WAITING);
    begin(0);
    char frame[512];
    assert(wire_status(&e, 7, frame, sizeof frame) > 0);
    assert(strncmp(frame, "CT1,7,", 6) == 0);
    assert(wire_status(&e, 7, frame, 8) == 0);
    assert(wire_crc("123456789", 9) == 0x29b1);
    puts("engine: timing, wrap, independent debounce, faults and CRC passed");
}
