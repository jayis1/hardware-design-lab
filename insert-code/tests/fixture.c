/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <assert.h>
#include "engine.h"
#include "wire.h"
int main(void)
{
    Engine e;
    engine_init(&e);
    const unsigned words[] = {0,3,5,1,0,12,0,1,3,0};
    uint32_t tick = 0;
    char frame[1024];
    for (unsigned i = 0; i < sizeof words / sizeof words[0]; ++i) {
        for (unsigned n = 0; n < 10; ++n) {
            engine_sample(&e, ++tick, words[i]);
            size_t length = wire_encode(&e, frame, sizeof frame);
            assert(length);
            assert(fwrite(frame, 1, length, stdout) == length);
        }
    }
    return 0;
}
