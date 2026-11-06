/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void tick(Engine *e, unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(e, e->now + 1, raw);
        char frame[512];
        size_t n = wire_encode(e, frame, sizeof frame);
        if (!n) { fputs("invalid fixture\n", stderr); return; }
        fwrite(frame, 1, n, stdout);
    }
}
int main(void)
{
    /* Synthetic contact traces, not a captured physical measurement. */
    Engine e;
    engine_init(&e);
    tick(&e, 10, 60);
    tick(&e, 14, 10);
    tick(&e, 12, 10);
    tick(&e, 13, 10);
    tick(&e, 5, 10);
    tick(&e, 13, 10);
    tick(&e, 12, 10);
    tick(&e, 14, 10);
    tick(&e, 10, 60);
    tick(&e, 14, 10);
    tick(&e, 10, 60);
    tick(&e, 14, 10);
    tick(&e, 12, 10);
    tick(&e, 8, 10); /* key removed with bolt in transit: terminal */
    return 0;
}
