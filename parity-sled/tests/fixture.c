/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic electrical words, not physical measurements. */
#include <assert.h>
#include <stdio.h>
#include "engine.h"
#include "wire.h"
static void emit(Engine *e)
{
    char line[1024];
    size_t n = wire_encode(e, line, sizeof line);
    assert(n);
    assert(fwrite(line, 1, n, stdout) == n);
}
static void hold(Engine *e, unsigned mask, unsigned n)
{
    while (n--) {
        engine_sample(e, e->now + 1, mask);
        emit(e);
    }
}
int main(int argc, char **argv)
{
    (void)argv;
    Engine e;
    engine_init(&e);
    if (argc > 1) {
        for (unsigned a = 0; a < 16; ++a)
            for (unsigned b = 0; b < 16; ++b) {
                hold(&e, a, 8);
                hold(&e, b, 8);
            }
    } else {
        for (unsigned mask = 0; mask < 16; ++mask)
            hold(&e, mask, 20);
        const unsigned words[] = {0,1,2,0,1,3,1};
        for (unsigned i = 0; i < sizeof words / sizeof words[0]; ++i)
            hold(&e, words[i], 20);
        engine_sample(&e, e.now + 11, 1);
        emit(&e);
        hold(&e, 1, 10);
        hold(&e, 0, 20);
        hold(&e, 15, 20);
    }
    return 0;
}
