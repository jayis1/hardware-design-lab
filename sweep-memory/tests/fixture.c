/* Author: jayis1. SPDX-License-Identifier: MIT. Synthetic fixture only. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Engine e;
static uint32_t tick;
static void emit(void)
{
    char line[1024];
    if (!wire_encode(&e,line,sizeof line)) exit(2);
    fputs(line,stdout);
}
static void hold(unsigned raw)
{
    for (unsigned n=0;n<8;n++) {
        engine_sample(&e,++tick,raw);
        emit();
    }
}
int main(void)
{
    const unsigned phases[4]={0,1,3,2};
    engine_init(&e);
    hold(0);
    for (unsigned p=1;p<=16;p++) hold(phases[p%4]);
    for (int p=15;p>=0;p--) hold(phases[p%4]|((p%2 ? 2u : 1u)<<2));
    return e.completed==1 ? 0 : 1;
}
