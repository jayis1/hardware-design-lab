/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "indicator.h"
#include "board.h"
/* Patterns use incremental ages rather than absolute modulo time so that
 * the cadence remains defined across a millisecond counter wrap. */
void indicator_poll(const Engine *e, uint32_t now)
{
    static uint32_t last;
    static uint32_t age;
    static Phase phase = PH_SETTLE;
    uint32_t delta = now - last;
    last = now;
    if (phase != e->phase) {
        phase = e->phase;
        age = 0;
    }
    age = engine_sat_add(age, delta);
    uint32_t period;
    uint32_t on_ms;
    switch (phase) {
    case PH_ARMED:
        period = 1000;
        on_ms = 500;
        break;
    case PH_TRANSFER:
        period = 250;
        on_ms = 125;
        break;
    case PH_COMPLETE:
        board_led(true);
        return;
    case PH_FAULT:
        period = 150;
        on_ms = 50;
        break;
    case PH_READY:
    case PH_SETTLE:
    default:
        period = 2000;
        on_ms = 50;
        break;
    }
    age %= period;
    board_led(age < on_ms);
}
