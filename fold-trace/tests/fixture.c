/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic states, serialized by the real production encoder. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
static Engine e;
static uint32_t time_ms;
static uint32_t sequence;
static void run(unsigned duration, uint8_t mask)
{
    for (unsigned i = 0; i < duration; ++i) {
        engine_sample(&e, ++time_ms, mask);
        if (time_ms % 100 == 0) {
            char frame[256];
            size_t n = wire_status(frame, sizeof frame, &e, time_ms, sequence++, 0);
            fwrite(frame, 1, n, stdout);
        }
    }
}
int main(void)
{
    engine_init(&e, 0);
    run(1100, 0);
    for (unsigned station = 0; station < 4; ++station) {
        run(600, (uint8_t)(1u << station));
        run(100, 0);
    }
    run(1100, 0);
    run(100, 2);
    run(200, 2);
    return 0;
}
