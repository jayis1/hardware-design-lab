/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static uint32_t tick;
static void hold(unsigned raw, unsigned count)
{
    char output[768];
    while (count--) {
        engine_sample(&e, ++tick, (uint8_t)raw);
        if (tick % 10 == 0) {
            size_t size = wire_encode(&e, output, sizeof output);
            assert(size);
            assert(fwrite(output, 1, size, stdout) == size);
        }
    }
}
static void pulse(unsigned rung)
{
    hold(1u << rung, 10);
    hold(0, 10);
}
int main(void)
{
    engine_init(&e);
    hold(0, 260);
    pulse(3); pulse(2); pulse(1); pulse(0);
    pulse(0); pulse(1); pulse(1);
    hold(0, 100);
    return 0;
}
