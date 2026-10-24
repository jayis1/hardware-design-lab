/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void advance(Engine *e, uint32_t *t, uint8_t mask, unsigned ms)
{
    for (unsigned i = 0; i < ms; ++i)
        engine_sample(e, ++*t, mask);
}
static void ready(Engine *e, uint32_t *t)
{
    engine_init(e, *t);
    advance(e, t, 0, 1100);
    assert(e->state == READY);
}
int main(void)
{
    Engine e;
    uint32_t t = 0;
    ready(&e, &t);
    for (unsigned i = 0; i < 4; ++i) {
        advance(&e, &t, 1u << i, 600);
        advance(&e, &t, 0, 40);
    }
    assert(e.good == 1 && e.bad == 0 && e.state == CLEAR);
    advance(&e, &t, 0, 1100);
    advance(&e, &t, 2, 40);
    assert(e.reason == ORDER && e.bad == 1);
    ready(&e, &t);
    advance(&e, &t, 1, 100);
    advance(&e, &t, 0, 40);
    assert(e.reason == SHORT_HOLD && e.bad == 1);
    ready(&e, &t);
    advance(&e, &t, 3, 40);
    assert(e.reason == OVERLAP);
    ready(&e, &t);
    advance(&e, &t, 1, 40);
    advance(&e, &t, 1, 30100);
    assert(e.reason == TIMEOUT);
    ready(&e, &t);
    advance(&e, &t, 1, 10);
    advance(&e, &t, 0, 40);
    assert(e.state == READY && e.bad == 0);
    advance(&e, &t, 1, 40);
    t += 20;
    engine_sample(&e, t, 1);
    assert(e.reason == SAMPLE_GAP && e.gaps == 1);
    t = 0xfffffff0u;
    ready(&e, &t);
    assert(e.state == READY);
    char frame[256];
    size_t n = wire_status(frame, sizeof frame, &e, t, 42, 0);
    assert(n > 20 && frame[n-1] == '\n');
    assert(wire_status(frame, 4, &e, t, 42, 0) == 0);
    assert(wire_crc("123456789", 9) == 0x29b1);
    n = wire_status(frame, sizeof frame, &e, t, 42, 0);
    fwrite(frame, 1, n, stdout);
    puts("engine and encoder tests passed");
    return 0;
}
