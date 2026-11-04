/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Engine e;
static void emit(void)
{
    char frame[768];
    size_t n=wire_encode(&e,frame,sizeof frame);
    if(!n) exit(2);
    fwrite(frame,1,n,stdout);
}
static void hold(unsigned raw,unsigned ticks)
{
    while(ticks--) {
        engine_sample(&e,e.now+1,(uint8_t)raw);
        if(e.now%100==0) emit();
    }
}
int main(void)
{
    engine_init(&e); emit();
    hold(15,600);
    hold(14,100); hold(10,100); hold(8,100); hold(0,400);
    hold(15,600); hold(7,100); hold(15,100); hold(0,400);
    hold(15,600); hold(14,10100);
    emit();
    return 0;
}
