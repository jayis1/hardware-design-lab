/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"

int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Do not claim normal acquisition without watchdog recovery. */
        for (;;) { }
    }
    Engine engine;
    engine_init(&engine,board_millis());
    engine.home_attempted=!engine_reset_allows_home(board_reset_cause());
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
        /* Reporting never blocks the 1 ms acquisition loop. USART is TX only;
         * all policy lives in this firmware, not in untrusted host commands. */
        if (now-last_report>=250u && !serial_busy()) {
            last_report=now;
            size_t count=wire_status(&engine,sequence++,record,sizeof record);
            if (!count || !serial_submit(record,count)) {
                engine_transport_loss(&engine);
            }
        }
    }
}
