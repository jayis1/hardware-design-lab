/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

/* Sampling is never inside the UART transmit wait. The single pending
 * frame is bounded and may be abandoned after 100 ms. No hidden queue can
 * make old evidence appear current; device time travels with every frame. */
int main(void)
{
    if (!board_clock_reset()) {
        /* Clock failure: no transport or measurements can be trusted. */
        for (;;) { }
    }
    board_init();
    CW engine;
    cw_init(&engine, board_millis());
    bool watchdog = board_watchdog_start();
    if (!watchdog || !board_inputs_configured())
        cw_fault(&engine);
    uint32_t sampled = board_millis();
    uint32_t reported = sampled;
    uint32_t transmit_errors = 0;
    char frame[SERIAL_BUFFER];
    for (;;) {
        uint32_t now = board_millis();
        if (now != sampled) {
            cw_tick(&engine, now, board_closed());
            sampled = now;
            if (watchdog)
                board_watchdog_feed();
        }
        serial_poll();
        if (serial_take_timeout()) {
            if (transmit_errors < UINT32_MAX)
                ++transmit_errors;
            else
                cw_fault(&engine);
        }
        if (!serial_busy() && now - reported >= 50u) {
            size_t length = cw_encode(&engine, now, transmit_errors,
                                      frame, sizeof frame);
            if (!length) {
                cw_fault(&engine);
            } else if (!serial_submit(frame, length)) {
                if (transmit_errors < UINT32_MAX)
                    ++transmit_errors;
                else
                    cw_fault(&engine);
            }
            reported = now;
        }
    }
}
