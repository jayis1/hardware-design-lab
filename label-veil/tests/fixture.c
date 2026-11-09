/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic classification fixture, never participant or physical data. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine e;
static uint32_t tick;
static void hold(unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(&e, ++tick, raw);
        char buffer[768];
        size_t length = wire_encode(&e, buffer, sizeof buffer);
        assert(length);
        assert(fwrite(buffer, 1, length, stdout) == length);
    }
}
int main(void)
{
    engine_init(&e);
    hold(0, 8);
    for (unsigned truth=0; truth<2; ++truth) {
        for (unsigned answer=0; answer<2; ++answer) {
            unsigned dock = 1u | (truth << 1);
            hold(dock, 8);
            hold(dock | (4u << answer), 8);
            hold(dock, 8);
            hold(0, 8);
        }
    }
    hold(1, 8);
    hold(13, 8);
    hold(1, 8);
    hold(0, 8);
    return 0;
}
