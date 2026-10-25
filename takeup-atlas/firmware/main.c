/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"
#include <limits.h>

static Engine engine;
static char frame[SERIAL_BUFFER];
static uint32_t sequence;
static uint32_t errors;
static uint32_t last_frame;
static bool watchdog;

static void transport_error(void)
{
    if (errors != UINT32_MAX)
        ++errors;
}

static void transmit(uint32_t time)
{
    if (time - last_frame < 100u)
        return;
    last_frame = time;
    if (serial_busy()) {
        transport_error();
        return;
    }
    size_t length = wire_encode(&engine, sequence, errors, frame, sizeof frame);
    if (!length) {
        transport_error();
        return;
    }
    if (!serial_submit(frame, length)) {
        transport_error();
        return;
    }
    ++sequence;
}

static void acquire(void)
{
    uint32_t before = board_millis();
    if (before == engine.time)
        return;
    uint8_t phases = board_closed();
    uint32_t after = board_millis();
    /* A tick during the GPIO reads makes phase/time association uncertain.
     * Do not process it; the next call records a gap if necessary. */
    if (after != before)
        return;
    engine_sample(&engine, before, phases);
}

int main(void)
{
    board_init();
    watchdog = board_watchdog_start();
    uint32_t now = board_millis();
    engine_init(&engine, now, board_closed());
    if (!watchdog) {
        engine.saturated = true; /* Latch unusable data, never hide startup failure. */
        engine.state = FAULT;
    }
    last_frame = now - 100u;
    for (;;) {
        acquire();
        serial_poll();
        if (serial_take_timeout())
            transport_error();
        transmit(board_millis());
        if (watchdog)
            board_watchdog_feed();
        /* Busy polling keeps UART draining between 1ms samples. USB bench
         * operation is intentional; no claim of battery sleep current. */
    }
}
