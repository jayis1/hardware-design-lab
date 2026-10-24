/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "board.h"
#include "engine.h"
#include "wire.h"
/* Large graph is static, keeping interrupt and application stacks small. */
static Atlas atlas;
int main(void)
{
    board_init();
    if (!board_watchdog_start()) {
        /* Absent telemetry is a fault, never an empty/healthy switch map. */
        for (;;) { }
    }
    uint32_t now = board_millis();
    atlas_init(&atlas, now);
    uint32_t sampled = now;
    uint32_t reported = now;
    uint32_t sequence = 0;
    uint32_t loss = 0;
    char frame[SERIAL_BUFFER];
    AtlasEvent event;
    for (;;) {
        now = board_millis();
        if (now != sampled) {
            atlas_sample(&atlas, now, board_closed());
            sampled = now;
            /* Feeding requires sampling progress, not serial activity. */
            board_watchdog_feed();
        }
        serial_poll();
        if (serial_take_timeout())
            loss = atlas_add(loss, 1);
        if (serial_busy())
            continue;
        size_t length = 0;
        if (now - reported >= 100u) {
            reported = now;
            length = wire_status(frame, sizeof frame, &atlas, now,
                                 sequence++, loss);
        } else if (atlas_pop(&atlas, &event)) {
            length = wire_event(frame, sizeof frame, &event, sequence++);
        } else {
            continue;
        }
        if (!length || !serial_submit(frame, length))
            loss = atlas_add(loss, 1);
    }
}
