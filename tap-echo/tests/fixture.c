/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Engine engine;
static uint32_t tick;
static void emit(void)
{
    char buffer[1024];
    size_t length = wire_encode(&engine, buffer, sizeof buffer);
    if (!length || fwrite(buffer, 1, length, stdout) != length)
        exit(1);
}
static void hold(unsigned mask, unsigned duration)
{
    while (duration--) {
        engine_sample(&engine, ++tick, mask);
        if (tick % 100 == 0) emit();
    }
}
int main(void)
{
    engine_init(&engine);
    hold(4, 10); hold(12, 10);
    hold(13, 20); hold(12, 80); hold(14, 20); hold(12, 80); hold(4, 10);
    hold(0, 10); hold(8, 10);
    hold(9, 20); hold(8, 80); hold(10, 20); hold(8, 80); hold(0, 10);
    emit();
    return 0;
}
