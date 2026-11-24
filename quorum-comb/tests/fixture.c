/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine e;
    engine_init(&e);
    /* Synthetic fixture: each complete truth-table state, never real data. */
    for (unsigned raw = 0; raw < 16; ++raw) {
        for (unsigned i = 0; i < 100; ++i) {
            engine_sample(&e, e.now + 1, raw);
            if ((i % 10) == 9) {
                char frame[1024];
                size_t n = wire_encode(&e, frame, sizeof frame);
                if (!n || fwrite(frame, 1, n, stdout) != n)
                    return 1;
            }
        }
    }
    return 0;
}
