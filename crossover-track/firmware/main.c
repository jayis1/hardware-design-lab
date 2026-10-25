/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Never advertise acquisition after watchdog clock failure. */
        for (;;) { }
    }
    Engine engine;
    engine_init(&engine, board_millis());
    char frame[SERIAL_BUFFER];
    uint32_t previous_sample = engine.now;
    uint32_t previous_frame = engine.now;
    uint32_t sequence = 0;
    uint32_t reported_result = 0;
    Phase reported_phase = WAITING;
    bool first = true;
    for (;;) {
        serial_poll();
        if (serial_take_timeout()) {
            engine_transport_loss(&engine);
            first = true;
        }
        uint32_t now = board_millis();
        if (now != previous_sample) {
            previous_sample = now;
            engine_sample(&engine, now, board_closed());
            board_watchdog_feed();
        }
        bool changed = reported_result != engine.last.id;
        changed |= reported_phase != engine.phase;
        bool due = now - previous_frame >= 100u;
        if (!serial_busy() && (first || changed || due)) {
            size_t length = wire_status(&engine, sequence, frame, sizeof frame);
            if (length && serial_submit(frame, length)) {
                ++sequence;
                previous_frame = now;
                reported_result = engine.last.id;
                reported_phase = engine.phase;
                first = false;
            } else {
                engine_transport_loss(&engine);
                previous_frame = now;
            }
        }
    }
}
