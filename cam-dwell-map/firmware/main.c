/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    if (!board_clock_reset()) {
        /* No measurements or serial claims under an unknown clock tree. */
        for (;;) { }
    }
    board_init();
    Cam engine;
    cam_init(&engine, board_millis());
    bool watchdog = board_watchdog_start();
    if (!watchdog || !board_inputs_configured())
        cam_fault(&engine);
    uint32_t sampled = board_millis();
    uint32_t reported = sampled;
    uint32_t transmit_errors = 0;
    char frame[SERIAL_BUFFER];
    for (;;) {
        uint32_t now = board_millis();
        if (now != sampled) {
            cam_tick(&engine, now, board_closed());
            sampled = now;
            if (watchdog)
                board_watchdog_feed();
        }
        serial_poll();
        if (serial_take_timeout()) {
            if (transmit_errors < UINT32_MAX)
                ++transmit_errors;
            else
                cam_fault(&engine);
        }
        /* Every snapshot repeats fixed policy marker and last complete map.
         * Late attachment does not depend on receiving a boot packet. */
        if (!serial_busy() && now - reported >= 100u) {
            size_t length = cam_encode(&engine, now, transmit_errors,
                                       frame, sizeof frame);
            if (!length) {
                cam_fault(&engine);
            } else if (!serial_submit(frame, length)) {
                if (transmit_errors < UINT32_MAX)
                    ++transmit_errors;
                else
                    cam_fault(&engine);
            }
            reported = now;
        }
    }
}
