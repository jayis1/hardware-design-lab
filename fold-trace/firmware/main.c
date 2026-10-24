/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"
#include <limits.h>
int main(void)
{
    board_init();
    /* Without a working watchdog refuse to begin inspection. */
    if (!board_watchdog_start())
        for (;;) { }
    Engine engine;
    uint32_t now = board_millis();
    engine_init(&engine, now);
    uint32_t sampled = now;
    uint32_t reported = now;
    uint32_t sequence = 0;
    uint32_t loss = 0;
    char frame[SERIAL_BUFFER];
    for (;;) {
        now = board_millis();
        if (now != sampled) {
            engine_sample(&engine, now, board_closed());
            sampled = now;
            board_watchdog_feed();
        }
        serial_poll();
        if (serial_take_timeout() && loss < UINT32_MAX)
            ++loss;
        if (now - reported >= 100u) {
            reported = now;
            size_t n = wire_status(frame, sizeof frame, &engine, now, sequence++, loss);
            if (!n || !serial_submit(frame, n)) {
                if (loss < UINT32_MAX)
                    ++loss;
            }
        }
    }
}
