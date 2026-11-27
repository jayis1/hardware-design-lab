/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic acquired configurations, not physical measurements. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
int main(int argc, char **argv)
{
    const bool dense = argc == 2 && strcmp(argv[1], "--dense") == 0;
    Engine e;
    engine_init(&e);
    const unsigned words[] = {0, 3, 7, 11, 15, 1};
    char line[1024];
    for (unsigned i = 0; i < sizeof words / sizeof words[0]; ++i) {
        for (unsigned n = 0; n < 160; ++n) {
            engine_sample(&e, e.now + 1, (uint8_t)words[i]);
            if (dense || n % 20 == 19) {
                assert(wire_encode(&e, line, sizeof line));
                fputs(line, stdout);
            }
        }
    }
}
