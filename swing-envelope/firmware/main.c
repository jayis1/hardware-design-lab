/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* No misleading normal operation without the recovery path. */
        for (;;) { }
    }
    Engine engine;
    engine_init(&engine,board_millis());
    uint32_t previous=engine.now;
    uint32_t last_report=previous;
    uint32_t sequence=0;
    char record[SERIAL_BUFFER];
    for (;;) {
        uint32_t now=board_millis();
        serial_poll();
        if (serial_take_timeout()) {
            engine_transport_loss(&engine);
        }
        if (now!=previous) {
            engine_sample(&engine,board_closed(),now);
            previous=now;
            board_watchdog_feed();
        }
        /* Single-owner foreground state: ISR only increments a tick. */
        if (now-last_report>=250u && !serial_busy()) {
            last_report=now;
            size_t n=wire_status(&engine,sequence++,record,sizeof record);
            if (!n || !serial_submit(record,n)) {
                engine_transport_loss(&engine);
            }
        }
    }
}
