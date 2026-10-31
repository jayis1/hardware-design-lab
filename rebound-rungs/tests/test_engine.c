/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t tick;
static void hold(Engine *e, unsigned mask, unsigned count)
{
    while (count--) engine_sample(e, ++tick, (uint8_t)mask);
}
static void setup(Engine *e)
{
    tick = 0;
    engine_init(e);
    hold(e, 0, 260);
    assert(e->state == READY);
}
static void pulse(Engine *e, unsigned rung)
{
    hold(e, 1u << rung, 6);
    hold(e, 0, 6);
}
static void descent(Engine *e)
{
    for (int rung = 3; rung >= 0; --rung) pulse(e, (unsigned)rung);
    assert(e->state == TURN);
}
int main(void)
{
    Engine e;
    char output[768];
    /* Independent list oracle: all four-event words after a valid descent.
     * Include continuing input after both successful and invalid terminals. */
    for (unsigned word = 0; word < 256; ++word) {
        setup(&e);
        descent(&e);
        int reached = -1;
        bool done = false, failed = false;
        unsigned code = word;
        for (unsigned step = 0; step < 4; ++step) {
            unsigned gate = code & 3u;
            code >>= 2;
            if (!done && !failed) {
                if (reached < 0) {
                    if (!gate) reached = 0;
                    else failed = true;
                } else if (gate == (unsigned)reached) {
                    done = true;
                } else if (gate == (unsigned)reached + 1u) {
                    reached = (int)gate;
                    done = gate == 3;
                } else {
                    failed = true;
                }
            }
            pulse(&e, gate);
            assert(engine_valid(&e));
            assert((e.state == DONE) == done);
            assert((e.state == FAULT) == failed);
            if (done) assert(e.high == (unsigned)reached);
        }
    }
    for (unsigned high = 0; high < 4; ++high) {
        setup(&e);
        descent(&e);
        for (unsigned rung = 0; rung <= high; ++rung) pulse(&e, rung);
        if (high < 3) pulse(&e, high);
        assert(e.state == DONE && e.high == high);
        assert(e.downmask == 15 && e.upmask == (1u << (high + 1)) - 1);
        assert(e.events == 4 + high + 1 + (high < 3));
        assert(engine_valid(&e));
        assert(wire_encode(&e, output, sizeof output));
        assert(!wire_encode(&e, output, 10));
        uint32_t age = e.age;
        hold(&e, 15, 30);
        assert(e.state == DONE && e.age == age);
    }
    /* Every wrong first rung and every descent skip is rejected. */
    for (unsigned bad = 0; bad < 3; ++bad) {
        setup(&e); pulse(&e, bad);
        assert(e.state == FAULT && (e.faults & F_ORDER));
    }
    for (unsigned expected = 0; expected < 3; ++expected) {
        for (unsigned bad = 0; bad < 4; ++bad) {
            if (bad == expected) continue;
            setup(&e);
            for (int r = 3; r > (int)expected; --r) pulse(&e, (unsigned)r);
            pulse(&e, bad);
            assert(e.state == FAULT);
        }
    }
    setup(&e); descent(&e); pulse(&e, 1);
    assert(e.state == FAULT);
    setup(&e); descent(&e); pulse(&e, 0); pulse(&e, 2);
    assert(e.state == FAULT);
    setup(&e); hold(&e, 3, 1);
    assert(e.state == FAULT && (e.faults & F_OVERLAP));
    setup(&e); pulse(&e, 3); engine_sample(&e, tick + 2, 0);
    assert(e.state == FAULT && e.gaps == 1);
    setup(&e); pulse(&e, 3); hold(&e, 0, 5000);
    assert(e.state == FAULT && (e.faults & F_TIMEOUT));
    setup(&e); hold(&e, 8, 3); assert(e.state == READY);
    hold(&e, 8, 1); assert(e.state == DOWN);
    /* Qualified all-clear is necessary, initial occupied gates cannot arm. */
    tick = 0; engine_init(&e); hold(&e, 8, 500);
    assert(e.state == WAIT);
    hold(&e, 0, 253); assert(e.state == WAIT);
    hold(&e, 0, 1); assert(e.state == READY);
    setup(&e); uint32_t now = e.now;
    engine_sample(&e, now, 8); assert(e.state == READY);
    /* Tick wrap remains ordinary elapsed time. */
    engine_init(&e); tick = UINT32_MAX - 300;
    hold(&e, 0, 260); descent(&e); pulse(&e, 0); pulse(&e, 0);
    assert(e.state == DONE && engine_valid(&e));
    e.tx_errors = UINT32_MAX; engine_tx_error(&e);
    assert(e.tx_errors == UINT32_MAX);
    Engine corrupt = e; corrupt.upmask = 5;
    assert(!engine_valid(&corrupt));
    assert(!wire_encode(&corrupt, output, sizeof output));
    setup(&e); descent(&e); pulse(&e, 0); pulse(&e, 0);
    corrupt=e; corrupt.age=5000; corrupt.end_ms=5000;
    assert(!engine_valid(&corrupt));
    corrupt=e; corrupt.down[2]=1;
    assert(!engine_valid(&corrupt));
    /* A completing edge at the deadline must lose to timeout. */
    setup(&e); descent(&e); pulse(&e, 0);
    while (e.age < LIMIT_MS - 4) hold(&e, 0, 1);
    hold(&e, 1, 4);
    assert(e.state==FAULT && e.faults==F_TIMEOUT && e.end_ms==0);
    assert(wire_crc("123456789",9) == 0x29b1);
    puts("engine ordering, four brackets, fault, debounce, wrap and encoder tests passed");
    return 0;
}
