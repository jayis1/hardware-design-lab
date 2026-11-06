/* Author: jayis1. SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <assert.h>
#include "engine.h"
#include "wire.h"
static Engine e;
static void emit(void)
{
    char data[768];
    size_t n = wire_encode(&e, data, sizeof data);
    assert(n);
    assert(fwrite(data, 1, n, stdout) == n);
}
static void hold(unsigned raw, unsigned n)
{
    while (n--) engine_sample(&e, e.now + 1, raw);
    emit();
}
static void pulse(unsigned p)
{
    hold(p | 8, 5); hold(p | 12, 5); hold(p | 8, 5);
}
int main(void)
{
    engine_init(&e);
    hold(1, 60); hold(9, 5);
    pulse(1); pulse(2); pulse(0); pulse(1);
    hold(14, 5); /* tied A -> B and INDEX: uncertain */
    hold(10, 5); hold(2, 5);
    assert(e.completed == 1 && e.pulses == 5);
}
