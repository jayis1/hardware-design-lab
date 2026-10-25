/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    board_init();
    /* If the oscillator/watchdog setup fails, refuse to report trials. */
    if (!board_watchdog_start()) {
        for (;;) { }
    }
    Engine engine;
    engine_init(&engine, board_millis());
    char frame[SERIAL_BUFFER];
    uint32_t previous_sample = engine.now;
    uint32_t previous_frame = engine.now;
    uint32_t sequence = 0;
    uint32_t reported_id = 0;
    Phase reported_phase = WAIT_BASELINE;
    uint8_t reported_levels = 0;
    bool first_frame = true;
    for (;;) {
        serial_poll();
        if (serial_take_timeout()) {
            engine_transport_loss(&engine);
            first_frame = true;
        }
        uint32_t now = board_millis();
        if (now != previous_sample) {
            previous_sample = now;
            engine_sample(&engine, now, board_closed());
            /* Serviced only after acquisition and state processing. */
            board_watchdog_feed();
        }
        bool changed = reported_id != engine.last.id;
        changed |= reported_phase != engine.phase;
        changed |= reported_levels != engine.levels;
        bool due = now - previous_frame >= 100u;
        if (!serial_busy() && (first_frame || changed || due)) {
            size_t length = wire_status(&engine, sequence, frame, sizeof frame);
            if (length && serial_submit(frame, length)) {
                ++sequence;
                previous_frame = now;
                reported_id = engine.last.id;
                reported_phase = engine.phase;
                reported_levels = engine.levels;
                first_frame = false;
            } else {
                engine_transport_loss(&engine);
                previous_frame = now;
            }
        }
    }
}
