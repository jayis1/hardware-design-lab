/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine engine;
    engine_init(&engine);
    /* Synthetic every-sample topology sweep, not measured switch behavior. */
    for (unsigned mask = 0; mask < 16; ++mask) {
        for (unsigned sample = 0; sample < 20; ++sample) {
            engine_sample(&engine, engine.now + 1, mask);
            char frame[768];
            size_t length = wire_encode(&engine, frame, sizeof frame);
            if (!length || fwrite(frame, 1, length, stdout) != length)
                return 1;
        }
    }
    return 0;
}
