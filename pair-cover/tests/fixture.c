/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
int main(void)
{
    Engine engine;
    engine_init(&engine);
    char frame[1024];
    uint32_t now = 0;
    /* Synthetic input, not a recording of physical switches. */
    const uint8_t masks[] = {0, 15, 3, 12, 5, 10, 6, 9};
    for (unsigned m = 0; m < sizeof masks; ++m) {
        for (unsigned i = 0; i < 201; ++i) {
            engine_sample(&engine, ++now, masks[m]);
            if (i % 20 == 0) {
                if (!wire_encode(&engine, frame, sizeof frame))
                    return 1;
                fputs(frame, stdout);
            }
        }
    }
    return 0;
}
