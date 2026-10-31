/* Author: jayis1. SPDX-License-Identifier: MIT. Synthetic contacts, not hardware. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static unsigned t;
static void hold(unsigned raw, unsigned n)
{
    while(n--) {
        engine_sample(&e,++t,(uint8_t)raw);
        char out[768];
        assert(wire_encode(&e,out,sizeof out));
        fputs(out,stdout);
    }
}
int main(void)
{
    engine_init(&e); hold(0,10); hold(8,10);
    hold(9,10); hold(8,10); hold(10,10); hold(8,10);
    hold(12,10); hold(8,10); hold(9,10); hold(8,10); hold(0,10);
    hold(8,10); hold(10,10); hold(8,10); hold(0,10);
    return 0;
}
