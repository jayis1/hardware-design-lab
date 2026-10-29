/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"
int main(void)
{
    if (!board_clock_reset()) {
        for (;;) { }
    }
    board_init();
    Engine engine;
    engine_init(&engine);
    if (!board_inputs_configured())
        engine_fault(&engine, F_CONFIG);
    if (!board_watchdog_start())
        engine_fault(&engine, F_WATCHDOG);
    uint32_t sampled = board_millis();
    uint32_t emitted = sampled;
    char frame[SERIAL_BUFFER];
    for (;;) {
        serial_poll();
        if (serial_take_timeout())
            engine_tx_error(&engine);
        uint32_t now = board_millis();
        if (now != sampled) {
            sampled = now;
            engine_sample(&engine, now, board_closed());
            board_watchdog_feed();
        }
        if (now - emitted >= 100u && !serial_busy()) {
            emitted = now;
            size_t length = wire_encode(&engine, frame, sizeof frame);
            if (!length || !serial_submit(frame, length))
                engine_tx_error(&engine);
        }
    }
}
