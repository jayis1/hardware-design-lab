/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint32_t clock_ms;
static void hold(Engine *e, uint8_t mask, unsigned duration)
{
    while (duration--) engine_sample(e, mask, ++clock_ms);
}
static void pulse(Engine *e, uint8_t mask)
{
    hold(e, mask, 8);
    hold(e, 0, 8);
}
static void east(Engine *e) { pulse(e, 1); pulse(e, 2); }
static void west(Engine *e) { pulse(e, 2); pulse(e, 1); }
static void init(Engine *e)
{
    clock_ms=0;
    engine_init(e, clock_ms);
    hold(e, 0, 8);
}
int main(void)
{
    Engine e;
    init(&e);
    east(&e);
    assert(e.crossings==1 && e.direction==1 && !e.period_valid);
    pulse(&e,8);
    west(&e);
    assert(e.right_seen==1 && e.right_missed==0);
    pulse(&e,4);
    east(&e);
    assert(e.left_seen==1 && e.period_valid && e.period_ms==96);
    assert(e.east_ms==16 && e.west_ms==16);
    west(&e);
    east(&e);
    assert(e.left_missed==1 && e.right_missed==1);
    uint32_t old=e.crossings;
    hold(&e, 3, 10);
    assert(e.faults==1 && !e.ready && !e.period_valid);
    hold(&e, 3, 10);
    assert(e.faults==1);
    hold(&e, 0, 8);
    east(&e);
    assert(e.crossings==old+1);
    /* A missed scheduler tick destroys continuity, not cumulative evidence. */
    clock_ms+=10;
    engine_sample(&e,0,clock_ms);
    assert(e.gaps==1 && !e.ready && !e.period_valid);
    hold(&e,0,8);
    pulse(&e,1);
    hold(&e,0,10001);
    assert(e.timeouts==1 && !e.period_valid);
    /* Short glitches never become qualified closures. */
    init(&e);
    hold(&e,1,2); hold(&e,0,8);
    assert(e.pending==0);
    /* Repeated same-direction passes must not become a period. */
    east(&e); east(&e);
    assert(e.faults==1 && !e.period_valid);
    /* Timestamp wrap preserves intervals. */
    clock_ms=UINT32_MAX-20;
    engine_init(&e,clock_ms); hold(&e,0,8); east(&e);
    assert(e.crossings==1 && e.east_ms==16);
    /* Saturated counters never wrap into innocent-looking zeros. */
    e.transport_losses=UINT32_MAX;
    engine_transport_loss(&e);
    assert(e.transport_losses==UINT32_MAX);
    char line[256];
    size_t n=wire_status(&e,12,line,sizeof line);
    assert(n>0 && n<sizeof line && line[n-1]=='\n');
    assert(!strncmp(line,"SE1,12,",7));
    assert(wire_status(&e,0,line,5)==0);
    assert(wire_crc("123456789",9)==0x29b1);
    puts("PASS engine: directional pairs, amplitude bands, missed bands, faults, gaps, timeout, glitch, wrap, saturation, wire");
}
