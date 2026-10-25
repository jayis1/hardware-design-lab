/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No valid acquisition is advertised after initialization failure. */
        for (;;) { }
    }
    Engine engine;
    engine_init(&engine, board_millis());
    char frame[SERIAL_BUFFER];
    uint32_t previous_sample = engine.now;
    uint32_t previous_frame = engine.now;
    uint32_t sequence = 0;
    uint32_t reported_epoch = 0;
    uint32_t reported_rejected = 0;
    uint8_t reported_levels = 0;
    bool reported_ready = false;
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
            board_watchdog_feed();
        }
        bool changed = reported_epoch != engine.epoch;
        changed |= reported_rejected != engine.rejected;
        changed |= reported_levels != engine.levels;
        changed |= reported_ready != engine.ready;
        bool due = now - previous_frame >= 100u;
        if (!serial_busy() && (first_frame || changed || due)) {
            size_t length = wire_status(&engine, sequence, frame, sizeof frame);
            if (length && serial_submit(frame, length)) {
                ++sequence;
                previous_frame = now;
                reported_epoch = engine.epoch;
                reported_rejected = engine.rejected;
                reported_levels = engine.levels;
                reported_ready = engine.ready;
                first_frame = false;
            } else {
                engine_transport_loss(&engine);
                previous_frame = now;
            }
        }
    }
}
