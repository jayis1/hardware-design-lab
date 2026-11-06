/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic switch history through the production engine and encoder. */
#include "engine.h"
#include "wire.h"
#include <assert.h>
#include <stdio.h>
static Engine e;
static void hold(unsigned raw, unsigned count)
{
    while (count--) {
        engine_sample(&e, e.now + 1, raw);
        if (!(e.now % 5)) {
            char line[768];
            size_t n = wire_encode(&e, line, sizeof line);
            assert(n);
            assert(fwrite(line, 1, n, stdout) == n);
        }
    }
}
int main(void)
{
    engine_init(&e);
    hold(5, 10);
    hold(4, 10); hold(6, 10); hold(4, 10); hold(5, 10);
    hold(1, 10); hold(9, 10); hold(1, 10); hold(5, 10);
    hold(1, 10); hold(9, 10); hold(1, 10); hold(5, 10);
    hold(0, 10); hold(10, 10); hold(0, 10); hold(5, 10);
    hold(4, 10); hold(5, 10);
    assert(e.alternating == 1 && e.repeated == 1 && e.ties == 1);
    assert(e.lane[0].canceled == 1);
    return 0;
}
