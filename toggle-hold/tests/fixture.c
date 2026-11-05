/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <stdlib.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static void emit(void)
{
    char frame[768];
    size_t n = wire_encode(&e, frame, sizeof frame);
    if (!n) exit(2);
    fwrite(frame, 1, n, stdout);
}
static void ticks(unsigned raw, unsigned n)
{
    while (n--) {
        engine_sample(&e, e.now + 1, raw);
        if (e.now % 10 == 0) emit();
    }
}
int main(int argc, char **argv)
{
    unsigned origin = argc > 1 && argv[1][0] == 'B' ? 2 : 1;
    unsigned dest = 3 ^ origin;
    unsigned push = origin == 1 ? 4 : 8;
    engine_init(&e);
    emit();
    ticks(origin, 60);
    ticks(origin | push, 10);
    ticks(push, 30);
    ticks(dest | push, 30);
    ticks(dest, 510);
    emit();
    return e.result == RETAINED ? 0 : 1;
}
