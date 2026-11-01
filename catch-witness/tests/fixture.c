/* Author: jayis1. SPDX-License-Identifier: MIT */
#include "engine.h"
#include "wire.h"
#include <stdio.h>
#include <assert.h>
static Engine e;
static void emit(void)
{
    char line[768]; assert(wire_encode(&e,line,sizeof line)); fputs(line,stdout);
}
static void run(unsigned raw, unsigned n)
{
    while(n--) {
        engine_sample(&e,e.now+1,(uint8_t)raw);
        if(e.now%100==0) emit();
    }
}
int main(void)
{
    engine_init(&e); emit(); run(0,60);
    run(3,210); run(0,60); /* Unexplained. */
    run(3,210); run(7,10); run(3,10); run(0,60); /* Handle-associated. */
    run(3,210); run(11,10); run(3,10); run(0,60); /* Tug-associated. */
    run(1,10); run(0,60); /* Never settled. */
    emit(); return 0;
}
