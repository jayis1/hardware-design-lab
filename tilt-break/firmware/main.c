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
    if (!adc_init())
        engine_fault(&engine, F_ADC);
    if (!board_watchdog_start())
        engine_fault(&engine, F_WATCHDOG);
    uint32_t sampled = board_millis();
    uint32_t emitted = sampled;
    uint32_t sequence = 0;
    char frame[SERIAL_BUFFER];
    for (;;) {
        serial_poll();
        if (serial_take_timeout())
            engine_fault(&engine, F_TX);
        uint32_t now = board_millis();
        if ((uint32_t)(now - sampled) >= PERIOD_MS) {
            sampled = now;
            uint16_t values[4];
            bool valid = adc_read(values);
            engine_sample(&engine, now, values, valid);
            board_watchdog_feed();
        }
        if ((uint32_t)(now - emitted) >= 200u && !serial_busy()) {
            emitted = now;
            size_t length = wire_encode(&engine, sequence, frame, sizeof frame);
            if (!length || !serial_submit(frame, length))
                engine_fault(&engine, F_TX);
            else
                ++sequence;
        }
    }
}
