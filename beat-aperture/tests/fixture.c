/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic endpoint signals, not measurements. Uses the production encoder. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Engine e;
static void emit(void)
{
    char frame[768];
    if(!wire_encode(&e,frame,sizeof frame)) exit(1);
    fputs(frame,stdout);
}
static void hold(unsigned raw,unsigned ticks)
{
    while(ticks--) engine_sample(&e,e.now+1,(uint8_t)raw);
}
static void pulse(unsigned raw)
{
    hold(raw,5); hold(0,5);
}
int main(void)
{
    engine_init(&e); hold(0,5); emit();
    pulse(5); emit(); /* A and B anchors */
    pulse(2); pulse(1); emit(); /* zero */
    pulse(8); pulse(4); pulse(2); pulse(1); emit(); /* one */
    pulse(8); pulse(4); pulse(8); pulse(4); pulse(2); pulse(1); emit(); /* many */
    return 0;
}
