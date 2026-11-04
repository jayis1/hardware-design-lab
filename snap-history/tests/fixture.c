/* Author: jayis1. SPDX-License-Identifier: MIT */
/* Synthetic fixture only; never represented as measured hardware data. */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <stdlib.h>
static Engine e;
static uint32_t now;
static void emit(void)
{
    char line[768];
    size_t n=wire_encode(&e,line,sizeof line);
    if (!n || fwrite(line,1,n,stdout)!=n) exit(1);
}
static void hold(unsigned mask,unsigned duration)
{
    while(duration--) engine_sample(&e,++now,(uint8_t)mask);
    emit();
}
int main(void)
{
    engine_init(&e);
    hold(0,255); hold(1,5); hold(3,25); hold(2,25);
    hold(10,25); hold(15,25); hold(15,250); hold(0,100);
    return e.phase==DONE?0:1;
}
