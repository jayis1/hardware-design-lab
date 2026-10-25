/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t now;
static void hold(Engine *e, uint8_t mask, unsigned ticks)
{
    while (ticks--) engine_sample(e, ++now, mask);
}
static void start(Engine *e)
{
    now = 0;
    engine_init(e, now);
    hold(e, 0, 12);
    assert(e->ready);
    hold(e, 8, 12);
    hold(e, 0, 12);
    assert(e->epoch == 1);
}
int main(void)
{
    Engine e;
    start(&e);
    hold(&e, 1, 108);
    assert(e.basket[0].total == 99);
    assert(e.basket[0].entries == 1 && e.basket[0].baths == 1);
    assert(e.basket[1].total == 0);
    hold(&e, 9, 12);
    assert(e.rejected == 1 && e.epoch == 1);
    hold(&e, 0, 12);
    uint32_t total = e.basket[0].total;
    hold(&e, 8, 12);
    assert(e.epoch == 2 && e.basket[0].epoch_ms == 0);
    assert(e.basket[0].total == total);
    hold(&e, 0, 12);
    hold(&e, 3, 20);
    assert(e.basket[0].baths == 2 && e.basket[1].baths == 1);
    assert(e.basket[0].entries == 2);
    total = e.basket[0].total;
    now += 100;
    engine_sample(&e, now, 3);
    assert(e.gaps == 1 && !e.ready);
    assert(e.basket[0].total == total && (e.basket[0].flags & 1));
    hold(&e, 3, 20);
    assert(!e.ready && e.basket[0].total == total);
    hold(&e, 0, 20);
    assert(e.ready);
    hold(&e, 1, 20);
    assert(e.basket[0].total > total);
    start(&e);
    /* A held change button at startup cannot invent a fresh bath. */
    engine_init(&e, now);
    hold(&e, 8, 30);
    assert(!e.ready && e.epoch == 0);
    hold(&e, 0, 20);
    hold(&e, 9, 20);
    assert(e.epoch == 0 && e.rejected == 1);
    /* An extraction and marker in one accepted vector is ambiguous. */
    start(&e);
    hold(&e, 1, 20);
    hold(&e, 8, 20);
    assert(e.epoch == 1 && e.rejected == 1);
    /* Unrelated contact movement does not restart debounce. */
    start(&e);
    hold(&e, 1, 5);
    hold(&e, 3, 4);
    assert((e.levels & 1) && !(e.levels & 2));
    hold(&e, 3, 5);
    assert((e.levels & 3) == 3);
    /* Qualified durations survive timestamp wrap without aliasing. */
    start(&e);
    now = UINT32_MAX - 20;
    e.now = now;
    hold(&e, 1, 50);
    assert(e.basket[0].total == 41);
    e.basket[0].total = UINT32_MAX - 1;
    hold(&e, 1, 4);
    assert(e.basket[0].total == UINT32_MAX && (e.basket[0].flags & 2));
    char frame[512];
    size_t n = wire_status(&e, 7, frame, sizeof frame);
    assert(n && frame[n-1] == '\n' && !memcmp(frame,"RL1,",4));
    assert(wire_status(&e, 7, frame, 10) == 0);
    assert(wire_crc("123456789",9) == 0x29b1);
    assert(wire_status(NULL,0,frame,sizeof frame) == 0);
    puts("PASS rinse accounting, independent debounce, ambiguous markers, gaps, rollover, saturation, CRC bounds");
}
