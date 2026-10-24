/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"
#include "drivers/indicator.h"
int main(void)
{
    Engine engine;
    char frame[SERIAL_BUFFER];
    uint32_t sequence = 0;
    uint32_t sampled = UINT32_MAX;
    uint32_t sent = 0;
    Phase reported = PH_SETTLE;
    bool dirty = true;
    board_init();
    engine_init(&engine);
    if (!board_watchdog_start()) {
        /* No measurements are emitted without a functioning watchdog. */
        for (;;) {
            board_led((board_millis() % 100u) < 50u);
        }
    }
    for (;;) {
        uint32_t now = board_millis();
        if (now != sampled) {
            engine_sample(&engine, now, board_closed());
            sampled = now;
            indicator_poll(&engine, now);
            /* Feed only after the sampling state machine has progressed. */
            board_watchdog_feed();
        }
        serial_poll();
        if (serial_take_timeout()) {
            engine_transport_loss(&engine);
            dirty = true;
        }
        if (engine.phase != reported)
            dirty = true;
        if ((dirty || now - sent >= 250u) && !serial_busy()) {
            size_t length = wire_encode(&engine, now, sequence,
                                        frame, sizeof frame);
            if (!length || !serial_submit(frame, length)) {
                engine_transport_loss(&engine);
            } else {
                ++sequence;
                reported = engine.phase;
                dirty = false;
                sent = now;
            }
        }
    }
}
